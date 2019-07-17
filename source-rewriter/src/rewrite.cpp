#include "rewrite.h"
#include "utils.h"

#include "ops/ops.h"
#include "ops/push.h"
#include "ops/pop.h"
#include "ops/broadcast.h"
#include "ops/scatter.h"
#include "ops/gather.h"
#include "ops/reduce.h"

#include <iostream>

using namespace clang;

struct Rewrite
{
public:
    Rewrite(OperationMetadata metadata, std::string callName, OperationExtractor* extractor)
    : metadata(std::move(metadata)), callName(std::move(callName)), extractor(extractor)
    {

    }

    OperationMetadata metadata;
    std::string callName;
    OperationExtractor* extractor;
};

class RewriteOpsVisitor: public RecursiveASTVisitor<RewriteOpsVisitor>
{
public:
    explicit RewriteOpsVisitor(Rewriter& rewriter) : rewriter(rewriter)
    {
        this->extractors.push_back(std::make_unique<PushExtractor>());
        this->extractors.push_back(std::make_unique<PushChannelExtractor>());
        this->extractors.push_back(std::make_unique<PopExtractor>());
        this->extractors.push_back(std::make_unique<PopChannelExtractor>());
        this->extractors.push_back(std::make_unique<BroadcastExtractor>());
        this->extractors.push_back(std::make_unique<BroadcastChannelExtractor>());
        this->extractors.push_back(std::make_unique<ReduceExtractor>());
        this->extractors.push_back(std::make_unique<ReduceChannelExtractor>());
        this->extractors.push_back(std::make_unique<GatherExtractor>());
        this->extractors.push_back(std::make_unique<GatherChannelExtractor>());
        this->extractors.push_back(std::make_unique<ScatterExtractor>());
        this->extractors.push_back(std::make_unique<ScatterChannelExtractor>());

        for (auto& extractor: this->extractors)
        {
            for (auto& fn: extractor->GetFunctionNames())
            {
                this->callMap[fn] = extractor.get();
            }
        }
    }

    bool VisitCallExpr(CallExpr* expr)
    {
        auto callee = expr->getDirectCallee();
        if (callee)
        {
            auto name = callee->getName().str();
            auto it = this->callMap.find(name);
            if (it != this->callMap.end())
            {
                auto& extractor = it->second;
                auto metadata = extractor->GetOperationMetadata(expr);

                this->rewrites.emplace_back(metadata, name, extractor);

                auto renamed = extractor->RenameCall(name, metadata);
                this->rewriter.ReplaceText(expr->getBeginLoc(), renamed);
            }
        }

        return true;
    }

    const std::vector<Rewrite>& getRewrites() const
    {
        return this->rewrites;
    }

private:
    Rewriter& rewriter;

    std::vector<std::unique_ptr<OperationExtractor>> extractors;
    std::unordered_map<std::string, OperationExtractor*> callMap;
    std::vector<Rewrite> rewrites;
};

/**
 * Only visit functions that are marked as user device kernels.
 */
bool RewriteKernelsVisitor::VisitFunctionDecl(FunctionDecl* f)
{
    bool isKernel = isKernelFunction(f);
    if (isKernel)
    {
        std::cerr << "SMI: rewriting function " << f->getName().str() << std::endl;

        RewriteOpsVisitor visitor(this->rewriter);
        visitor.TraverseFunctionDecl(f);

        for (auto& rewrite: visitor.getRewrites())
        {
            rewrite.extractor->OutputMetadata(rewrite.metadata, std::cout);
            std::cerr << "SMI: rewrote ";
            rewrite.extractor->OutputMetadata(rewrite.metadata, std::cerr);
            this->rewriter.InsertTextBefore(f->getBeginLoc(), rewrite.extractor->CreateDeclaration(rewrite.callName,
                    rewrite.metadata) + "\n");
        }
    }

    return false;
}
