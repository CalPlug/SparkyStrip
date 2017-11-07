void dft(int CH)
{	
// Debug
	float fft_value = 0.0;
	
// Initializes the initial maximum dft value
	max_dft_value[CH] = 0.0;
	
	// Calculates the DFT in complex buffer.  This algorythm also finds the maximum value of the DFT to normalize the values at 180 and 300 Hz
	for(i=0;i<BUFFER_LENGTH;i++)
	{
		// Initializes the complex buffer
		complex_buffer[i][CH].real = 0.0;
		complex_buffer[i][CH].imag = 0.0;
		for(j=0;j<BUFFER_LENGTH;j++)
		{
			complex_buffer[i][CH].real += input[j][CH]*((float) cos(2*PI*i*j/BUFFER_LENGTH));
			complex_buffer[i][CH].imag -= input[j][CH]*((float) sin(2*PI*i*j/BUFFER_LENGTH));
		}
		// Records the Highest DFT value
		if(max_dft_value[CH] < sqrt(complex_buffer[i][CH].real*complex_buffer[i][CH].real + complex_buffer[i][CH].imag*complex_buffer[i][CH].imag))
		{
			max_dft_value[CH] = sqrt(complex_buffer[i][CH].real*complex_buffer[i][CH].real + complex_buffer[i][CH].imag*complex_buffer[i][CH].imag);
		}	
	}
	
// Debug
	for(i=0;i<100;i++)
	{
		fft_value = sqrt(complex_buffer[i][CH].real*complex_buffer[i][CH].real + complex_buffer[i][CH].imag*complex_buffer[i][CH].imag)/max_dft_value[CH];
		//printf("FFT %d is %f\n", i, fft_value);
	}
		
	// Records the Normalized Harmonic values of each Channel at 180 and 300 Hz
	normalized_FFT_180[CH] = sqrt(complex_buffer[18][CH].real*complex_buffer[18][CH].real + complex_buffer[18][CH].imag*complex_buffer[18][CH].imag)/max_dft_value[CH];
	normalized_FFT_300[CH] = sqrt(complex_buffer[31][CH].real*complex_buffer[31][CH].real + complex_buffer[31][CH].imag*complex_buffer[31][CH].imag)/max_dft_value[CH];
	
}

/*
   This computes an in-place complex-to-complex FFT 
   x and y are the real and imaginary arrays of 2^m points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform 
*/
short FFT(short int dir,long m,double *x,double *y)
{
   long n,i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   n = 1;
   for (i=0;i<m;i++) 
      n *= 2;

   /* Do the bit reversal */
   i2 = n >> 1;
   j = 0;
   for (i=0;i<n-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0; 
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0; 
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<n;i+=l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1; 
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }
   
   return(TRUE);
}