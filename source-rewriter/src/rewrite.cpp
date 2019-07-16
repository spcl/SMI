#include <utility>

#include "rewrite.h"
#include "utils.h"

#include <iostream>

using namespace clang;

class FindVarDecl: public clang::RecursiveASTVisitor<FindVarDecl>
{
public:
    bool VisitVarDecl(VarDecl* decl)
    {
        assert(!this->decl);
        this->decl = decl;

        return false;
    }
    bool VisitDeclRefExpr(DeclRefExpr* expr)
    {
        this->TraverseDecl(expr->getDecl());
        return true;
    }

    VarDecl* decl = nullptr;
};

static VarDecl* extractChannelDecl(CallExpr* expr)
{
    assert(expr->getNumArgs() > 0);
    auto& arg = expr->getArgs()[0];
    FindVarDecl visitor;
    visitor.TraverseStmt(arg);
    assert(visitor.decl);
    return visitor.decl;
}

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

class RewriteOpsVisitor: public clang::RecursiveASTVisitor<RewriteOpsVisitor>
{
public:
    explicit RewriteOpsVisitor(clang::Rewriter& rewriter) : rewriter(rewriter)
    {
        this->callMap["SMI_Push"] = std::make_unique<PushExtractor>();
        this->callMap["SMI_Push_flush"] = std::make_unique<PushExtractor>();
        this->callMap["SMI_Pop"] = std::make_unique<PopExtractor>();
        this->callMap["SMI_Bcast"] = std::make_unique<BroadcastExtractor>();
        this->callMap["SMI_Scatter"] = std::make_unique<ScatterExtractor>();
        this->callMap["SMI_Gather"] = std::make_unique<GatherExtractor>();
        this->callMap["SMI_Reduce"] = std::make_unique<ReduceExtractor>();
    }

    bool VisitCallExpr(clang::CallExpr* expr)
    {
        auto callee = expr->getDirectCallee();
        if (callee)
        {
            auto name = callee->getName().str();
            auto it = this->callMap.find(name);
            if (it != this->callMap.end())
            {
                auto& extractor = it->second;
                auto metadata = extractor->GetOperationMetadata(extractChannelDecl(expr));

                this->rewrites.emplace_back(metadata, name, extractor.get());

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
    clang::Rewriter& rewriter;

    std::unordered_map<std::string, std::unique_ptr<OperationExtractor>> callMap;
    std::vector<Rewrite> rewrites;
};

/**
 * Only visit functions that are marked as user device kernels.
 */
bool RewriteKernelsVisitor::VisitFunctionDecl(clang::FunctionDecl* f)
{
    bool isKernel = isKernelFunction(f);
    if (isKernel)
    {
        std::cerr << "SMI: rewriting function " << f->getName().str() << std::endl;

        RewriteOpsVisitor visitor(this->rewriter);
        visitor.TraverseFunctionDecl(f);

        for (auto& rewrite: visitor.getRewrites())
        {
            rewrite.metadata.output(std::cout);
            this->rewriter.InsertTextBefore(f->getBeginLoc(), rewrite.extractor->CreateDeclaration(rewrite.callName,
                    rewrite.metadata) + "\n");
            std::cerr << "SMI: rewrote ";
            rewrite.metadata.output(std::cerr);
        }
    }

    return false;
}
