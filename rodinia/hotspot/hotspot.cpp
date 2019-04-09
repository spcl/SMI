#include "hotspot.h"
#include "common/timer.h"
#include "hlslib/intel/OpenCL.h"
#include "mpi.h"

cl_device_id *device_list;

void writeoutput(float *vect, int grid_rows, int grid_cols, char *file) {
  int i, j, index = 0;
  FILE *fp;
  char str[STR_SIZE];

  if ((fp = fopen(file, "w")) == 0) {
    printf("could not open output file, skipping...\n");
  } else {
    for (i = 0; i < grid_rows; i++) {
      for (j = 0; j < grid_cols; j++) {
        sprintf(str, "%d\t%f\n", index, vect[i * grid_cols + j]);
        fputs(str, fp);
        index++;
      }
    }
    fclose(fp);
  }
}

void readinput(float *vect, int grid_rows, int grid_cols, char *file) {
  int i, j;
  FILE *fp;
  char str[STR_SIZE];
  float val;

  printf("Reading %s\n", file);

  if ((fp = fopen(file, "r")) == 0) fatal("The input file was not opened");

  for (i = 0; i <= grid_rows - 1; i++)
    for (j = 0; j <= grid_cols - 1; j++) {
      if (fgets(str, STR_SIZE, fp) == NULL) fatal("Error reading file\n");
      if (feof(fp)) fatal("not enough lines in file");
      // if ((sscanf(str, "%d%f", &index, &val) != 2) || (index !=
      // ((i-1)*(grid_cols-2)+j-1)))
      if ((sscanf(str, "%f", &val) != 1)) fatal("invalid file format");
      vect[i * grid_cols + j] = val;
    }

  fclose(fp);
}

/*
  compute N time steps
*/

int compute_tran_temp(cl_mem MatrixPower, cl_mem MatrixTemp[2], int col,
                      int row, int total_iterations, bool run_smi, int smi_rank,
                      int smi_size) {
  float grid_height = chip_height / row;
  float grid_width = chip_width / col;

  float Cap = FACTOR_CHIP * SPEC_HEAT_SI * t_chip * grid_width * grid_height;
  float Rx = grid_width / (2.0 * K_SI * t_chip * grid_height);
  float Ry = grid_height / (2.0 * K_SI * t_chip * grid_width);
  float Rz = t_chip / (K_SI * grid_height * grid_width);

  float max_slope = MAX_PD / (FACTOR_CHIP * t_chip * SPEC_HEAT_SI);
  float step = PRECISION / max_slope;

  int src = 0, dst = 1;

  TimeStamp compute_start, compute_end;
  double computeTime;

  CL_SAFE_CALL(clFinish(command_queue));

  {
    // Using the single work-item versions.
    // All iterations are computed by a single kernel execution.
    // haloCols and haloRows are not used in these versions, so
    // they are not passed.
    {
      // Exit condition should be a multiple of comp_bsize
      int comp_bsize = BLOCK_X - 2;
      int last_col = (col % comp_bsize == 0)
                         ? col + 0
                         : col + comp_bsize - col % comp_bsize;
      int col_blocks = last_col / comp_bsize;
      int comp_exit = BLOCK_X * col_blocks * (row + 1) / SSIZE;

      float step_div_Cap = step / Cap;
      float Rx_1 = 1 / Rx;
      float Ry_1 = 1 / Ry;
      float Rz_1 = 1 / Rz;

      CL_SAFE_CALL(
          clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&MatrixPower));
      CL_SAFE_CALL(clSetKernelArg(kernel, 3, sizeof(int), (void *)&col));
      CL_SAFE_CALL(clSetKernelArg(kernel, 4, sizeof(int), (void *)&row));
      CL_SAFE_CALL(
          clSetKernelArg(kernel, 5, sizeof(float), (void *)&step_div_Cap));
      CL_SAFE_CALL(clSetKernelArg(kernel, 6, sizeof(float), (void *)&Rx_1));
      CL_SAFE_CALL(clSetKernelArg(kernel, 7, sizeof(float), (void *)&Ry_1));
      CL_SAFE_CALL(clSetKernelArg(kernel, 8, sizeof(float), (void *)&Rz_1));
      CL_SAFE_CALL(
          clSetKernelArg(kernel, 9, sizeof(float), (void *)&comp_exit));
    }

    // Beginning of timing point
    GetTime(compute_start);

    // Launch kernel
    if (!run_smi) {
      for (int t = 0; t < total_iterations; ++t) {
        CL_SAFE_CALL(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                    (void *)&MatrixTemp[src]));
        CL_SAFE_CALL(clSetKernelArg(kernel, 2, sizeof(cl_mem),
                                    (void *)&MatrixTemp[dst]));
        CL_SAFE_CALL(clEnqueueTask(command_queue, kernel, 0, NULL, NULL));

        clFinish(command_queue);

        src = 1 - src;
        dst = 1 - dst;
      }
    } else {
      for (int t = 0; t < total_iterations; ++t) {
        CL_SAFE_CALL(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                    (void *)&MatrixTemp[0]));
        CL_SAFE_CALL(clSetKernelArg(kernel, 2, sizeof(cl_mem),
                                    (void *)&MatrixTemp[0]));
        CL_SAFE_CALL(
            clSetKernelArg(kernel, 10, sizeof(float), (void *)&t));
        CL_SAFE_CALL(clEnqueueTask(command_queue, kernel, 0, NULL, NULL));

        clFinish(command_queue);

        src = 1 - src;
        dst = 1 - dst;
      }
    }
  }

  // Wait for all operations to finish
  clFinish(command_queue);

  GetTime(compute_end);

  computeTime = TimeDiff(compute_start, compute_end);
  printf("\nComputation done in %0.3lf ms.\n", computeTime);
  printf("Throughput is %0.3lf GBps.\n",
         (3 * row * col * sizeof(float) * total_iterations) /
             (1000000000.0 * computeTime / 1000.0));

  return src;
}

void usage(int argc, char **argv) {
  fprintf(stderr,
          "Usage: %s <grid_rows/grid_cols> <kernel_file> <sim_time> "
          "<temp_file> <power_file> <output_file>\n",
          argv[0]);
  fprintf(stderr,
          "\t<grid_rows/grid_cols>  - number of rows/cols in the grid "
          "(positive integer)\n");
  fprintf(stderr, "\t<run_smi> - run SMI version (bool)\n");
  fprintf(stderr, "\t<sim_time>   - number of iterations\n");
  fprintf(stderr,
          "\t<temp_file>  - name of the file containing the initial "
          "temperature values of each cell\n");
  fprintf(stderr,
          "\t<power_file> - name of the file containing the dissipated power "
          "values of each cell\n");
  fprintf(stderr, "\t<output_file> - name of the output file (optional)\n");

  fprintf(stderr,
          "\tNote: If output file name is not supplied, output will not be "
          "written to disk.\n");
  exit(1);
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int smi_rank, smi_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &smi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &smi_size);

  int device_id = smi_rank % 2;

  int write_out = 0;

  int size;
  int grid_rows, grid_cols = 0;
  float *FilesavingTemp = NULL, *FilesavingPower = NULL;
  char *tfile, *pfile, *ofile = NULL;

  int total_iterations = 60;

  if (argc < 5) usage(argc, argv);
  if ((grid_rows = atoi(argv[1])) <= 0 || (grid_cols = atoi(argv[1])) <= 0 ||
      (total_iterations = atoi(argv[3])) < 0)
    usage(argc, argv);

  std::string kernel_file(argv[2]);
  bool run_smi = false;
  bool emulator = false;

  if (kernel_file.find("smi") != std::string::npos) {
    printf("Running SMI implementation.\n");
    run_smi = true;
  } else {
    printf("Running non-SMI implementation.\n");
  }

  if (kernel_file.find("emulat") != std::string::npos) {
    setenv("CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA", "1", false);
    emulator = true;
  } else {
    emulator = false;
  }

  if (run_smi && emulator) {
    kernel_file = kernel_file.substr(0, kernel_file.find(".aocx")) + "_" +
                  std::to_string(smi_rank) + ".aocx";
  }

  printf("Using kernel file %s\n", kernel_file.c_str());

  size_t devices_size;
  cl_int result, error;
  cl_uint platformCount;
  cl_platform_id *platforms = NULL;
  cl_context_properties ctxprop[3];
  cl_device_type device_type;

  TimeStamp total_start, total_end;

  hlslib::ocl::Context context_hlslib(device_id);
  cl_context context = context_hlslib.context()();

  // get the list of GPUs
  result =
      clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &devices_size);
  int num_devices = (int)(devices_size / sizeof(cl_device_id));

  if (result != CL_SUCCESS || num_devices < 1) {
    printf("ERROR: clGetContextInfo() failed\n");
    return -1;
  }
  device_list = (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices);
  if (!device_list) {
    printf("ERROR: new cl_device_id[] failed\n");
    return -1;
  }
  CL_SAFE_CALL(clGetContextInfo(context, CL_CONTEXT_DEVICES, devices_size,
                                device_list, NULL));
  device = device_list[0];

  // Create command queue
  command_queue =
      clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error);
  if (error != CL_SUCCESS) fatal_CL(error, __LINE__);

  tfile = argv[4];
  pfile = argv[5];
  if (argc >= 7) {
    write_out = 1;
    ofile = argv[6];
  }

  size = grid_rows * grid_cols;

  FilesavingTemp = (float *)alignedMalloc(size * sizeof(float));
  FilesavingPower = (float *)alignedMalloc(size * sizeof(float));

  if (!FilesavingPower || !FilesavingTemp) fatal("unable to allocate memory");

  // Read input data from disk
  readinput(FilesavingTemp, grid_rows, grid_cols, tfile);
  readinput(FilesavingPower, grid_rows, grid_cols, pfile);

  size_t sourceSize;
  char *source = read_kernel(kernel_file.c_str(), &sourceSize);

  cl_program program =
      clCreateProgramWithBinary(context, 1, device_list, &sourceSize,
                                (const unsigned char **)&source, NULL, &error);
  if (error != CL_SUCCESS) fatal_CL(error, __LINE__);

  char clOptions[110];
  sprintf(clOptions, "-I.");

  // Create an executable from the kernel
  clBuildProgram_SAFE(program, 1, &device, clOptions, NULL, NULL);

  // Create kernel
  kernel = clCreateKernel(program, "hotspot", &error);
  if (error != CL_SUCCESS) fatal_CL(error, __LINE__);

  GetTime(total_start);

  // Create two temperature matrices and copy the temperature input data
  cl_mem MatrixTemp[2], MatrixPower = NULL;

  // Create input memory buffers on device
  if (run_smi) {
    MatrixTemp[0] = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   2 * sizeof(float) * size, NULL, &error);
    if (error != CL_SUCCESS) fatal_CL(error, __LINE__);

    CL_SAFE_CALL(clEnqueueWriteBuffer(command_queue, MatrixTemp[0], CL_TRUE, 0,
                                      sizeof(float) * size, FilesavingTemp, 0,
                                      NULL, NULL));
    CL_SAFE_CALL(clEnqueueWriteBuffer(
        command_queue, MatrixTemp[0], CL_TRUE, sizeof(float) * size,
        sizeof(float) * size, FilesavingTemp, 0, NULL, NULL));
  } else {
    MatrixTemp[0] = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   sizeof(float) * size, NULL, &error);
    if (error != CL_SUCCESS) fatal_CL(error, __LINE__);
    MatrixTemp[1] = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   sizeof(float) * size, NULL, &error);
    if (error != CL_SUCCESS) fatal_CL(error, __LINE__);

    CL_SAFE_CALL(clEnqueueWriteBuffer(command_queue, MatrixTemp[0], CL_TRUE, 0,
                                      sizeof(float) * size, FilesavingTemp, 0,
                                      NULL, NULL));
  }

  // Copy the power input data
  MatrixPower = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * size,
                               NULL, &error);
  if (error != CL_SUCCESS) fatal_CL(error, __LINE__);

  CL_SAFE_CALL(clEnqueueWriteBuffer(command_queue, MatrixPower, CL_TRUE, 0,
                                    sizeof(float) * size, FilesavingPower, 0,
                                    NULL, NULL));

  // Perform the computation
  int ret = compute_tran_temp(MatrixPower, MatrixTemp, grid_cols, grid_rows,
                              total_iterations, run_smi, smi_rank, smi_size);

  // Copy final temperature data back
  float *MatrixOut = NULL;

  MatrixOut = (float *)alignedMalloc(sizeof(float) * size);
  if (run_smi) {
    int offset = (total_iterations % 2 == 0) ? 0 : grid_rows * grid_cols;
    CL_SAFE_CALL(clEnqueueReadBuffer(
        command_queue, MatrixTemp[0], CL_TRUE, sizeof(float) * offset,
        sizeof(float) * size, MatrixOut, 0, NULL, NULL));
  } else {
    CL_SAFE_CALL(clEnqueueReadBuffer(command_queue, MatrixTemp[ret], CL_TRUE, 0,
                                     sizeof(float) * size, MatrixOut, 0, NULL,
                                     NULL));
  }

  GetTime(total_end);
  printf("Total run time was %f ms.\n", TimeDiff(total_start, total_end));

  // Write final output to output file
  if (write_out) {
    writeoutput(MatrixOut, grid_rows, grid_cols, ofile);
  }

  clReleaseMemObject(MatrixTemp[0]);
  if (!run_smi) {
    clReleaseMemObject(MatrixTemp[1]);
  }
  clReleaseMemObject(MatrixPower);

  clReleaseContext(context);

  return 0;
}
