class Matrix {
  public static void main(String args[])
  {
    if (args.length != 1) {
      System.out.println("usage: java Matrix N");
      return;
    }
    int n = Integer.parseInt(args[0]);

    double[][] a = new double[n][n]; // Matrix A
    double[][] b = new double[n][n]; // Matrix B
    double[][] c = new double[n][n]; // Matrix C

    // Initialize the matrices to some values.
    int i, j;
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        a[i][j] = i * n + j;
        b[i][j] = j * n + i;
        c[i][j] = 0;
      }
    }

    long begin = System.currentTimeMillis();

    /**************************************/
    /* Write code to calculate C = A * B. */
    /**************************************/

    long end = System.currentTimeMillis();
    System.out.printf("time: %.6f sec\n", (end - begin) / 1000.0);

    // Print C for debugging. Comment out the print before measuring the execution time.
    double sum = 0;
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        sum += c[i][j];
        // System.out.printf("c[%d][%d]=%f\n", i, j, c[i][j]);
      }
    }
    // Print out the sum of all values in C.
    // This should be 450 for N=3, 3680 for N=4, and 18250 for N=5.
    System.out.printf("sum: %.6f\n", sum);
  }
}
