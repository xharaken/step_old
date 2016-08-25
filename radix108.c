/*radixsort with 8 thread*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <assert.h>
#include "lib-kessyou.c"

#define NTDQ 0
#define SIMD 0

#define DELTA_KEY 0
#define DELTA_MOVE 0

/* memory for data (data size * 2)*/
unsigned data[50*1024*1024]__attribute__((aligned(16*16)));

#define THREAD 16
#define SPECIAL 256
#define STAGE 40

/* memory for special use (16KB)*/
#if NTDQ
unsigned special[THREAD][SPECIAL]__attribute__((aligned(16*16)));
#elif SIMD
unsigned special[THREAD][SPECIAL]__attribute__((aligned(16*16)));
#else
unsigned special[THREAD][SPECIAL];
#endif

unsigned thread_id[THREAD];
unsigned hist[THREAD+1];
unsigned left[THREAD];
unsigned right[THREAD];
unsigned long long *keyleft[THREAD];
unsigned long long *keyright[THREAD];
unsigned *count[THREAD];
unsigned status[STAGE][THREAD];
double record[STAGE][THREAD];
unsigned n,m,nth,keytype,shift,width;
unsigned *src,*dst;

inline void prepare(void);
inline void print_data(unsigned *data);
inline void print_result(int sup);
inline void barrier(volatile unsigned *state,int id);
inline void* radixsort(void *argtmp);
inline void key_norm_r3(unsigned id);
inline void key_maxv_r3(unsigned id);
inline void cnt1_r3(unsigned id);
inline void rdx1_r3(unsigned id);
inline void cnt2_r3(unsigned id);
inline void rdx2_r3(unsigned id);
inline void cnt3_r3(unsigned id);
inline void rdx3_r3(unsigned id);
inline void move_r3(unsigned id);
inline void key_norm_r4(unsigned id);
inline void key_maxv_r4(unsigned id);
inline void cnt1_r4(unsigned id);
inline void rdx1_r4(unsigned id);
inline void cnt2_r4(unsigned id);
inline void rdx2_r4(unsigned id);
inline void cnt3_r4(unsigned id);
inline void rdx3_r4(unsigned id);
inline void cnt4_r4(unsigned id);
inline void rdx4_r4(unsigned id);
inline void move_r4(unsigned id);
inline void sum1_8(unsigned id);
inline void sum2_8(unsigned id);
inline void sum3_8(unsigned id);
inline void sum1_10(unsigned id);
inline void sum2_10(unsigned id);
inline void sum3_10(unsigned id);
inline void sum1_11(unsigned id);
inline void sum2_11(unsigned id);
inline void sum3_11(unsigned id);

int main(unsigned argc,char **argv)
{
  unsigned problem,i;
  pthread_t handle[THREAD];
  
  if(argc != 3)
    {
      fprintf(stderr,"Usage : %s problem thread\n",argv[0]);
      exit(1);
    }
  problem = atoi(argv[1]);
  nth = atoi(argv[2]);
  if(!(1 <= problem) || !(1 <= nth))
    {
      fprintf(stderr,"Usage : %s problem thread r\n",argv[0]);
      exit(1);
    }

  generate_list((float*)data,(int)problem,(int*)&n,(int*)&m,(int*)&keytype);
  
  //print_data(data);
  
  prepare();
  
  for(i=0; i<nth; i++)
    {
      thread_id[i] = i;
      pthread_create(handle+i,NULL,radixsort,(void*)(thread_id+i));
    }
  
  for(i=0; i<nth; i++)
    {
      pthread_join(handle[i],NULL);
    }
  
  //print_data(data);
  
  check_list((float*)data,(int)problem,(int)n,(int)m,(int)keytype);

  printf("\n");
  return 0;
}

inline void prepare(void)
{
  unsigned i,j;
  
  for(i=0; i<STAGE; i++)
    {
      for(j=0; j<THREAD; j++)
        {
          status[i][j] = 0;
        }
    }
  for(i=0; i<THREAD; i++)
    {
      for(j=0; j<SPECIAL; j++)
        {
          special[i][j] = 0;
        }
    }
  for(i=0; i<THREAD+1; i++)
    {
      hist[i] = 0;
    }
  for(shift=0; shift<32; shift++)
    {
      if((m+1)&1<<shift)
        {
          break;
        }
    }
  
  src = (unsigned*)data;
  dst = (unsigned*)(data+(n<<shift));
  
  width = n/nth;

  for(i=0; i<nth; i++)
    {
      left[i] = width*i;
      right[i] = width*(i+1);
      keyleft[i] = (unsigned long long*)(src+(left[i]<<shift));
      keyright[i] = (unsigned long long*)(src+(right[i]<<shift)-width*2);
    }
  //right[nth-1] = n;
}

inline void print_data(unsigned *data)
{
  unsigned i,j;

  printf("\n");
  for(i=0; i<n; i++)
    {
      for(j=0; j<m+1; j++)
        {
          printf("%f ",data[i*(m+1)+j]);
        }
      printf("\n");
    }
  printf("*******\n");
  fflush(stdout);
  return;
}

inline void print_result(int sup)
{
  unsigned i,j;
  double max_time,tmp,bw_sum;

  /*
  printf("\n");
  for(i=0; i<sup; i+=2)
    {
      if(i == sup-2)
        {
          printf("[move] ");
        }
      else
        {
          switch(i)
            {
            case 0:
              printf("[keys] ");
              break;
            case 2:
              printf("[cnt1] ");
              break;
            case 4:
              printf("[sum1] ");
              break;
            case 6:
              printf("[rdx1] ");
              break;
            case 8:
              printf("[cnt2] ");
              break;
            case 10:
              printf("[sum2] ");
              break;
            case 12:
              printf("[rdx2] ");
              break;
            case 14:
              printf("[cnt3] ");
              break;
            case 16:
              printf("[sum3] ");
              break;
            case 18:
              printf("[rdx3] ");
              break;
            case 20:
              printf("[cnt4] ");
              break;
            case 22:
              printf("[sum4] ");
              break;
            case 24:
              printf("[rdx4] ");
              break;
            default:
              assert(0);
            }
        }
      max_time = -1;
      for(j=0; j<nth; j++)
        {
          tmp = record[i+1][j]-record[i][j];
          if(tmp > max_time)
            {
              max_time = tmp;
            }
          //printf("(%d)=%.5lf~%.5lf ",j,record[i][j]-floor(record[i][j]),record[i+1][j]-floor(record[i+1][j]));
        }
      printf("time=%.5lf ",max_time);
      bw_sum = 0;
      for(j=0; j<nth; j++)
        {
          bw_sum += (right[j]-left[j])*(m+1)*sizeof(unsigned)/(record[i+1][j]-record[i][j])/1024/1024;
        }
      printf("bw=%.2lf\n",bw_sum);
    }
  */
  max_time = -1;
  for(j=0; j<nth; j++)
    {
      tmp = record[sup][j]-record[0][j];
      if(tmp > max_time)
        {
          max_time = tmp;
        }
      printf("(%d)=%.5lf ",j,tmp);
    }
  printf("time=%.5lf\n",max_time);
  fflush(stdout);
}

inline void barrier(volatile unsigned *status,int id)
{
  unsigned i,succeed;

  asm("mfence");
  status[id] = 1;
  asm("mfence");
  while(1)
    {
      succeed = 1;
      for(i=0; i<nth; i++)
        {
          if(status[i] != 1)
            {
              succeed = 0;
            }
        }
      if(succeed == 1)
        {
          break;
        }
    }
  asm("mfence");
}

inline void* radixsort(void *arg)
{
  register unsigned id = *(unsigned*)arg;
  cpu_set_t mask;
  struct sched_param sched;
  unsigned i;
  int policy = SCHED_FIFO;

#ifdef CPU_ZERO
  CPU_ZERO(&mask);
  CPU_SET(id,&mask);
  if(sched_setaffinity(0,sizeof(mask),&mask) == -1)
    {
      fprintf(stderr,"affinity error!");
    }
  
  /*
  sched.sched_priority = sched_get_priority_max(policy);
  if(sched_setscheduler(0,policy,&sched) == -1)
    {
      fprintf(stderr,"scheduler error!");
    }
  */
#endif
  
  if(m == 3 || m >= 255)
    {
      count[id] = special[id];
      
      barrier(status[0],id);
  
      barrier(status[1],id);
      record[0][id] = my_clock();
      
      if(keytype == KEYTYPE_NORM)
        {
          key_norm_r4(id);
        }
      else if(keytype == KEYTYPE_MAXV)
        {
          key_maxv_r4(id);
        }

      record[1][id] = my_clock();
      barrier(status[2],id);
      record[2][id] = my_clock();
      
      //cnt1_r4(id);
      
      record[3][id] = my_clock();
      barrier(status[3],id);
      record[4][id] = my_clock();
  
      sum1_8(id);
      barrier(status[4],id);
      sum2_8(id);
      barrier(status[5],id);
      sum3_8(id);

      record[5][id] = my_clock();
      barrier(status[6],id);
      record[6][id] = my_clock();
  
      rdx1_r4(id);
      
      record[7][id] = my_clock();
      barrier(status[7],id);
      record[8][id] = my_clock();
      
      cnt2_r4(id);
      
      record[9][id] = my_clock();
      barrier(status[8],id);
      record[10][id] = my_clock();

      sum1_8(id);
      barrier(status[9],id);
      sum2_8(id);
      barrier(status[10],id);
      sum3_8(id);
  
      record[11][id] = my_clock();
      barrier(status[11],id);
      record[12][id] = my_clock();

      rdx2_r4(id);
      
      record[13][id] = my_clock();
      barrier(status[12],id);
      record[14][id] = my_clock();

      cnt3_r4(id);
      
      record[15][id] = my_clock();
      barrier(status[13],id);
      record[16][id] = my_clock();

      sum1_8(id);
      barrier(status[14],id);
      sum2_8(id);
      barrier(status[15],id);
      sum3_8(id);
  
      record[17][id] = my_clock();
      barrier(status[16],id);
      record[18][id] = my_clock();

      rdx3_r4(id);
      
      record[19][id] = my_clock();
      barrier(status[17],id);
      record[20][id] = my_clock();
      
      cnt4_r4(id);
      
      record[21][id] = my_clock();
      barrier(status[18],id);
      record[22][id] = my_clock();

      sum1_8(id);
      barrier(status[19],id);
      sum2_8(id);
      barrier(status[20],id);
      sum3_8(id);
  
      record[23][id] = my_clock();
      barrier(status[21],id);
      record[24][id] = my_clock();

      rdx4_r4(id);
      
      record[25][id] = my_clock();
      barrier(status[22],id);
      record[26][id] = my_clock();

      move_r4(id);
      
      record[27][id] = my_clock();
      barrier(status[23],id);
      record[28][id] = my_clock();

      barrier(status[24],id);
      if(id == 0)
        {
          print_result(28);
        }
    }
  else
    {
      count[id] = (unsigned*)(src+(((left[id]+right[id])/2)<<shift));
      
      barrier(status[0],id);
      
      barrier(status[1],id);
      record[0][id] = my_clock();
  
      if(keytype == KEYTYPE_NORM)
        {
          key_norm_r3(id);
        }
      else if(keytype == KEYTYPE_MAXV)
        {
          key_maxv_r3(id);
        }

      record[1][id] = my_clock();
      barrier(status[2],id);
      record[2][id] = my_clock();
      
      cnt1_r3(id);
      
      record[3][id] = my_clock();
      barrier(status[3],id);
      record[4][id] = my_clock();
  
      sum1_11(id);
      barrier(status[4],id);
      sum2_11(id);
      barrier(status[5],id);
      sum3_11(id);

      record[5][id] = my_clock();
      barrier(status[6],id);
      record[6][id] = my_clock();
  
      rdx1_r3(id);
      
      record[7][id] = my_clock();
      barrier(status[7],id);
      record[8][id] = my_clock();

      cnt2_r3(id);
      
      record[9][id] = my_clock();
      barrier(status[8],id);
      record[10][id] = my_clock();

      sum1_11(id);
      barrier(status[9],id);
      sum2_11(id);
      barrier(status[10],id);
      sum3_11(id);
  
      record[11][id] = my_clock();
      barrier(status[11],id);
      record[12][id] = my_clock();

      rdx2_r3(id);
      
      record[13][id] = my_clock();
      barrier(status[12],id);
      record[14][id] = my_clock();

      cnt3_r3(id);
      
      record[15][id] = my_clock();
      barrier(status[13],id);
      record[16][id] = my_clock();

      sum1_10(id);
      barrier(status[14],id);
      sum2_10(id);
      barrier(status[15],id);
      sum3_10(id);
  
      record[17][id] = my_clock();
      barrier(status[16],id);
      record[18][id] = my_clock();

      rdx3_r3(id);
      
      record[19][id] = my_clock();
      barrier(status[17],id);
      record[20][id] = my_clock();

      move_r3(id);

      record[21][id] = my_clock();
      barrier(status[18],id);
      record[22][id] = my_clock();
      
      barrier(status[19],id);
      
      if(id == 0)
        {
          print_result(22);
        }
    }
  
  return NULL;
}

inline void key_norm_r3(unsigned id)
{
#if NTDQ
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)keyleft[id];

  if(m == 7)
    {
      for(i=left[id]; i<right[id];)
        {
          asm("movaps (%0),%%xmm0"::"r"(src_p));
          asm("movaps (%0),%%xmm1"::"r"(src_p+4));
          asm("movaps (%0),%%xmm2"::"r"(src_p+8));
          asm("movaps (%0),%%xmm3"::"r"(src_p+12));
          
          asm("movaps %xmm0,%xmm4");
          asm("movaps %xmm1,%xmm5");
          asm("mulps %xmm4,%xmm4");
          asm("mulps %xmm5,%xmm5");
          asm("addps %xmm5,%xmm4");
          asm("haddps %xmm4,%xmm4");
          asm("haddps %xmm4,%xmm4");
          asm("movss %xmm4,%xmm0");
          
          asm("movaps %xmm2,%xmm6");
          asm("movaps %xmm3,%xmm7");
          asm("mulps %xmm6,%xmm6");
          asm("mulps %xmm7,%xmm7");
          asm("addps %xmm7,%xmm6");
          asm("haddps %xmm6,%xmm6");
          asm("haddps %xmm6,%xmm6");
          asm("movss %xmm6,%xmm2");
          
          asm("movntps %%xmm0,(%0)"::"r"(dst_p));
          asm("movntps %%xmm1,(%0)"::"r"(dst_p+4));
          asm("movntps %%xmm2,(%0)"::"r"(dst_p+8));
          asm("movntps %%xmm3,(%0)"::"r"(dst_p+12));
          
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p));
          *(keyindex_p+1) = i;
          
          asm("movss %%xmm2,(%0)"::"r"(keyindex_p+2));
          *(keyindex_p+3) = i+1;
          
          i += 2;
          keyindex_p += 4;
          
          src_p += 16;
          dst_p += 16;
        }
    }
  else
    {
      for(i=left[id]; i<right[id];)
        {
          asm("movaps (%0),%%xmm0"::"r"(src_p));
          asm("movaps (%0),%%xmm1"::"r"(src_p+4));
          asm("movaps (%0),%%xmm2"::"r"(src_p+8));
          asm("movaps (%0),%%xmm3"::"r"(src_p+12));
          
          asm("movaps %%xmm1,(%0)"::"r"(dst_p+4));
          asm("movaps %%xmm2,(%0)"::"r"(dst_p+8));
          asm("movaps %%xmm3,(%0)"::"r"(dst_p+12));
      
          asm("movaps %xmm0,%xmm7");
          asm("mulps %xmm7,%xmm7");
          asm("mulps %xmm1,%xmm1");
          asm("addps %xmm1,%xmm7");
          asm("mulps %xmm2,%xmm2");
          asm("addps %xmm2,%xmm7");
          asm("mulps %xmm3,%xmm3");
          asm("addps %xmm3,%xmm7");
          
          for(j=16; j<m; j+=16)
            {
              asm("movaps (%0),%%xmm1"::"r"(src_p+j));
              asm("movaps (%0),%%xmm2"::"r"(src_p+j+4));
              asm("movaps (%0),%%xmm3"::"r"(src_p+j+8));
              asm("movaps (%0),%%xmm4"::"r"(src_p+j+12));
              
              asm("movntps %%xmm1,(%0)"::"r"(dst_p+j));
              asm("movntps %%xmm2,(%0)"::"r"(dst_p+j+4));
              asm("movntps %%xmm3,(%0)"::"r"(dst_p+j+8));
              asm("movntps %%xmm4,(%0)"::"r"(dst_p+j+12));
              
              asm("mulps %xmm1,%xmm1");
              asm("addps %xmm1,%xmm7");
              asm("mulps %xmm2,%xmm2");
              asm("addps %xmm2,%xmm7");
              asm("mulps %xmm3,%xmm3");
              asm("addps %xmm3,%xmm7");
              asm("mulps %xmm4,%xmm4");
              asm("addps %xmm4,%xmm7");
            }
          
          asm("haddps %xmm7,%xmm7");
          asm("haddps %xmm7,%xmm7");
          asm("movss %xmm7,%xmm0");
          
          asm("movaps %%xmm0,(%0)"::"r"(dst_p));
      
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p));
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p++));
          *keyindex_p++ = i++;
          
          src_p += j;
          dst_p += j;
        }
    }
#elif SIMD
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  
  for(i=left[id]; i<right[id];)
    {
      asm("movdqa (%0),%%xmm0"::"r"(src_p));
      asm("movdqa %%xmm0,(%0)"::"r"(dst_p));
      asm("mulps %xmm0,%xmm0");
      for(j=4; j<m; j+=4)
        {
          asm("movdqa (%0),%%xmm1"::"r"(src_p+j));
          asm("movdqa %%xmm1,(%0)"::"r"(dst_p+j));
          asm("mulps %xmm1,%xmm1");
          asm("addps %xmm1,%xmm0");
        }
      asm("haddps %xmm0,%xmm0");
      asm("haddps %xmm0,%xmm0");
      asm("movss %%xmm0,(%0)"::"r"(dst_p));
      asm("movss %%xmm0,(%0)"::"r"(keyindex_p++));
      *keyindex_p++ = i++;
      
      src_p += j;
      dst_p += j;
    }
#else
  register unsigned i,j;
  register float *src_p = (float*)(src+(left[id]<<shift));
  register float *dst_p = (float*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  register float norm;
  
  for(i=left[id]; i<right[id]; i++)
    {
      norm = 0;
      for(j=1; j<m+1; j++)
        {
          norm += *(src_p+j)**(src_p+j);
          *(dst_p+j) = *(src_p+j);
        }
      *dst_p = norm;
      *keyindex_p++ = *(unsigned*)(&norm);
      *keyindex_p++ = i;
      src_p += j;
      dst_p += j;
    }
#endif
}

inline void key_maxv_r3(unsigned id)
{
#if NTDQ
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));

  if(m == 7)
    {
      for(i=left[id]; i<right[id];)
        {
          asm("movaps (%0),%%xmm0"::"r"(src_p));
          asm("movaps (%0),%%xmm1"::"r"(src_p+4));
          asm("movaps (%0),%%xmm2"::"r"(src_p+8));
          asm("movaps (%0),%%xmm3"::"r"(src_p+12));
          
          asm("movaps %xmm0,%xmm4");
          asm("maxps %xmm1,%xmm4");
          asm("movaps %xmm2,%xmm5");
          asm("maxps %xmm3,%xmm5");
          
          asm("pshufd $0x1b,%xmm4,%xmm6");
          asm("maxps %xmm6,%xmm4");
          asm("pshufd $0xe1,%xmm4,%xmm6");
          asm("maxps %xmm6,%xmm4");
          asm("movss %xmm4,%xmm0");
          
          asm("pshufd $0x1b,%xmm5,%xmm6");
          asm("maxps %xmm6,%xmm5");
          asm("pshufd $0xe1,%xmm5,%xmm6");
          asm("maxps %xmm6,%xmm5");
          asm("movss %xmm5,%xmm2");
          
          asm("movntps %%xmm0,(%0)"::"r"(dst_p));
          asm("movntps %%xmm1,(%0)"::"r"(dst_p+4));
          asm("movntps %%xmm2,(%0)"::"r"(dst_p+8));
          asm("movntps %%xmm3,(%0)"::"r"(dst_p+12));
          
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p));
          *(keyindex_p+1) = i;
          asm("movss %%xmm2,(%0)"::"r"(keyindex_p+2));
          *(keyindex_p+3) = i+1;
          
          i += 2;
          keyindex_p += 4;
          
          src_p += 16;
          dst_p += 16;
        }
    }
  else if(m == 15)
    {
      for(i=left[id]; i<right[id];)
        {
          asm("prefetchnta (%0)"::"r"(src_p+DELTA_KEY));
          
          asm("movaps (%0),%%xmm0"::"r"(src_p));
          asm("movaps (%0),%%xmm1"::"r"(src_p+4));
          asm("movaps (%0),%%xmm2"::"r"(src_p+8));
          asm("movaps (%0),%%xmm3"::"r"(src_p+12));
          
          asm("movaps %xmm0,%xmm4");
          asm("maxps %xmm1,%xmm4");
          asm("maxps %xmm2,%xmm4");
          asm("maxps %xmm3,%xmm4");
          
          asm("pshufd $0x1b,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("pshufd $0xe1,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("movss %xmm4,%xmm0");
          
          asm("movntps %%xmm0,(%0)"::"r"(dst_p));
          asm("movntps %%xmm1,(%0)"::"r"(dst_p+4));
          asm("movntps %%xmm2,(%0)"::"r"(dst_p+8));
          asm("movntps %%xmm3,(%0)"::"r"(dst_p+12));
          
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p++));
          *keyindex_p++ = i++;
          
          src_p += 16;
          dst_p += 16;
        }
    }
  else if(m == 31)
    {
      for(i=left[id]; i<right[id];)
        {
          asm("movaps (%0),%%xmm0"::"r"(src_p));
          asm("movaps (%0),%%xmm1"::"r"(src_p+4));
          asm("movaps (%0),%%xmm2"::"r"(src_p+8));
          asm("movaps (%0),%%xmm3"::"r"(src_p+12));

          asm("movaps %%xmm1,(%0)"::"r"(dst_p+4));
          asm("movaps %%xmm2,(%0)"::"r"(dst_p+8));
          asm("movaps %%xmm3,(%0)"::"r"(dst_p+12));

          asm("movaps %xmm0,%xmm7");
          asm("maxps %xmm1,%xmm7");
          asm("maxps %xmm2,%xmm7");
          asm("maxps %xmm3,%xmm7");
          
          asm("movaps (%0),%%xmm1"::"r"(src_p+16));
          asm("movaps (%0),%%xmm2"::"r"(src_p+20));
          asm("movaps (%0),%%xmm3"::"r"(src_p+24));
          asm("movaps (%0),%%xmm4"::"r"(src_p+28));

          asm("movntps %%xmm1,(%0)"::"r"(dst_p+16));
          asm("movntps %%xmm2,(%0)"::"r"(dst_p+20));
          asm("movntps %%xmm3,(%0)"::"r"(dst_p+24));
          asm("movntps %%xmm4,(%0)"::"r"(dst_p+28));
          
          asm("maxps %xmm1,%xmm7");
          asm("maxps %xmm2,%xmm7");
          asm("maxps %xmm3,%xmm7");
          asm("maxps %xmm4,%xmm7");

          asm("pshufd $0x1b,%xmm7,%xmm6");
          asm("maxps %xmm6,%xmm7");
          asm("pshufd $0xe1,%xmm7,%xmm6");
          asm("maxps %xmm6,%xmm7");
          
          asm("movss %xmm7,%xmm0");
          asm("movaps %%xmm0,(%0)"::"r"(dst_p));
          
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p++));
          *keyindex_p++ = i++;
          
          src_p += 32;
          dst_p += 32;
        }
    }
  else
    {
      for(i=left[id]; i<right[id];)
        {
          asm("movaps (%0),%%xmm0"::"r"(src_p));
          asm("movaps (%0),%%xmm1"::"r"(src_p+4));
          asm("movaps (%0),%%xmm2"::"r"(src_p+8));
          asm("movaps (%0),%%xmm3"::"r"(src_p+16));
          
          asm("movaps %%xmm1,(%0)"::"r"(dst_p+4));
          asm("movaps %%xmm2,(%0)"::"r"(dst_p+8));
          asm("movaps %%xmm3,(%0)"::"r"(dst_p+12));
          
          asm("movaps %xmm0,%xmm7");
          asm("maxps %xmm1,%xmm7");
          asm("maxps %xmm2,%xmm7");
          asm("maxps %xmm3,%xmm7");
          
          for(j=16; j<m; j+=16)
            {
              asm("movaps (%0),%%xmm1"::"r"(src_p+j));
              asm("movaps (%0),%%xmm2"::"r"(src_p+j+4));
              asm("movaps (%0),%%xmm3"::"r"(src_p+j+8));
              asm("movaps (%0),%%xmm4"::"r"(src_p+j+12));
              
              asm("maxps %xmm1,%xmm7");
              asm("maxps %xmm2,%xmm7");
              asm("maxps %xmm3,%xmm7");
              asm("maxps %xmm4,%xmm7");

              asm("movntps %%xmm1,(%0)"::"r"(dst_p+j));
              asm("movntps %%xmm2,(%0)"::"r"(dst_p+j+4));
              asm("movntps %%xmm3,(%0)"::"r"(dst_p+j+8));
              asm("movntps %%xmm4,(%0)"::"r"(dst_p+j+12));
            }
          
          asm("pshufd $0x1b,%xmm7,%xmm6");
          asm("maxps %xmm6,%xmm7");
          asm("pshufd $0xe1,%xmm7,%xmm6");
          asm("maxps %xmm6,%xmm7");
          
          asm("movss %xmm7,%xmm0");
          asm("movaps %%xmm0,(%0)"::"r"(dst_p));
          
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p++));
          *keyindex_p++ = i++;
          
          src_p += j;
          dst_p += j;
        }
    }
#elif SIMD
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  
  for(i=left[id]; i<right[id];)
    {
      asm("movdqa (%0),%%xmm0"::"r"(src_p));
      asm("movdqa %%xmm0,(%0)"::"r"(dst_p));
      for(j=4; j<m; j+=4)
        {
          asm("movdqa (%0),%%xmm1"::"r"(src_p+j));
          asm("movdqa %%xmm1,(%0)"::"r"(dst_p+j));
          asm("maxps %xmm1,%xmm0");
        }
      asm("movdqa %xmm0,%xmm1");
      asm("shufps $0x1b,%xmm0,%xmm0");
      asm("maxps %xmm1,%xmm0");
      asm("movdqa %xmm0,%xmm1");
      asm("shufps $0xe1,%xmm0,%xmm0");
      asm("maxps %xmm1,%xmm0");
      asm("movss %%xmm0,(%0)"::"r"(dst_p));
      asm("movss %%xmm0,(%0)"::"r"(keyindex_p++));
      *keyindex_p++ = i++;
      src_p += j;
      dst_p += j;
    }
#else
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned maxv;
  
  for(i=left[id]; i<right[id]; i++)
    {
      maxv = *(src_p+1);
      *(dst_p+1) = *(src_p+1);
      for(j=2; j<m+1; j++)
        {
          if(*(src_p+j) > maxv)
            {
              maxv = *(src_p+j);
            }
          *(dst_p+j) = *(src_p+j);
        }
      *dst_p = maxv;
      *keyindex_p++ = maxv;
      *keyindex_p++ = i;
      src_p += j;
      dst_p += j;
    }
#endif
}

inline void cnt1_r3(unsigned id)
{
  register unsigned i;
  register unsigned long long *keyindex_p = keyleft[id];
  register unsigned *count_p = count[id];
  
  for(i=0; i<1<<11; i++)
    {
      count_p[i] = 0;
    }
  i = width;
  while(i--)
    {
      count_p[*keyindex_p&0x7ff]++;
      keyindex_p++;
    }
}

inline void rdx1_r3(unsigned id)
{
  register unsigned i = width,offset,tmp;
  register unsigned long long *keyindex_p = keyleft[id];
  register unsigned *count_p = count[id];
  
  while(i--)
    {
      offset = count_p[*keyindex_p&0x7ff]++;
      tmp = offset/width;
      *(keyright[tmp]+offset-tmp*width) = *keyindex_p++;
    }
}

inline void cnt2_r3(unsigned id)
{
  register unsigned i;
  register unsigned long long *keyindex_p = keyright[id];
  register unsigned *count_p = count[id];
  
  for(i=0; i<1<<11; i++)
    {
      count_p[i] = 0;
    }
  i = width;
  while(i--)
    {
      count_p[(*keyindex_p>>11)&0x7ff]++;
      keyindex_p++;
    }
}

inline void rdx2_r3(unsigned id)
{
  register unsigned i = width,offset,tmp;
  register unsigned long long *keyindex_p = keyright[id];
  register unsigned *count_p = count[id];
  
  while(i--)
    {
      offset = count_p[(*keyindex_p>>11)&0x7ff]++;
      tmp = offset/width;
      *(keyleft[tmp]+offset-tmp*width) = *keyindex_p++;
    }
}

inline void cnt3_r3(unsigned id)
{
  register unsigned i;
  register unsigned long long *keyindex_p = keyleft[id];
  register unsigned *count_p = count[id];
  
  for(i=0; i<1<<10; i++)
    {
      count_p[i] = 0;
    }
  i = width;
  while(i--)
    {
      count_p[(*keyindex_p>>22)&0x3ff]++;
      keyindex_p++;
    }
}

inline void rdx3_r3(unsigned id)
{
  register unsigned i = width,offset,tmp;
  register unsigned long long *keyindex_p = keyleft[id];
  register unsigned *count_p = count[id];
  
  while(i--)
    {
      offset = count_p[(*keyindex_p>>22)&0x3ff]++;
      tmp = offset/width;
      *(keyright[tmp]+offset-tmp*width) = *keyindex_p++;
    }
}

inline void move_r3(unsigned id)
{
#if NTDQ
  register unsigned i = width,j;
  register unsigned *src_p = (unsigned*)keyleft[id],*dst_p;
  register unsigned long long *keyindex_p = keyright[id];

  if(m == 7)
    {
      while(i)
        {
          dst_p = dst+((*keyindex_p>>32)<<shift);
          
          asm("movaps (%0),%%xmm0"::"r"(dst_p));
          asm("movaps (%0),%%xmm1"::"r"(dst_p+4));
          
          dst_p = dst+((*(keyindex_p+1)>>32)<<shift);
          
          asm("movaps (%0),%%xmm2"::"r"(dst_p));
          asm("movaps (%0),%%xmm3"::"r"(dst_p+4));
          
          asm("movntps %%xmm0,(%0)"::"r"(src_p));
          asm("movntps %%xmm1,(%0)"::"r"(src_p+4));
          asm("movntps %%xmm2,(%0)"::"r"(src_p+8));
          asm("movntps %%xmm3,(%0)"::"r"(src_p+12));
          
          src_p += 16;
          
          i -= 2;
          keyindex_p += 2;
        }
    }
  else if(m == 15)
    {
      while(i)
        {
          dst_p = dst+((*keyindex_p>>32)<<shift);
          
          /**/asm("prefetchnta (%0)"::"r"(dst+((*(keyindex_p+DELTA_MOVE)>>32)<<shift)));
          
          asm("movaps (%0),%%xmm0"::"r"(dst_p));
          asm("movaps (%0),%%xmm1"::"r"(dst_p+4));
          asm("movaps (%0),%%xmm2"::"r"(dst_p+8));
          asm("movaps (%0),%%xmm3"::"r"(dst_p+12));
          asm("movntps %%xmm0,(%0)"::"r"(src_p));
          asm("movntps %%xmm1,(%0)"::"r"(src_p+4));
          asm("movntps %%xmm2,(%0)"::"r"(src_p+8));
          asm("movntps %%xmm3,(%0)"::"r"(src_p+12));

          src_p += 16;

          i--;
          keyindex_p++;
        }
    }
  else if(m == 31)
    {
      while(i)
        {
          dst_p = dst+((*keyindex_p>>32)<<shift);
          
          asm("movaps (%0),%%xmm0"::"r"(dst_p));
          asm("movaps (%0),%%xmm1"::"r"(dst_p+4));
          asm("movaps (%0),%%xmm2"::"r"(dst_p+8));
          asm("movaps (%0),%%xmm3"::"r"(dst_p+12));
          asm("movaps (%0),%%xmm4"::"r"(dst_p+16));
          asm("movaps (%0),%%xmm5"::"r"(dst_p+20));
          asm("movaps (%0),%%xmm6"::"r"(dst_p+24));
          asm("movaps (%0),%%xmm7"::"r"(dst_p+28));
          asm("movntps %%xmm0,(%0)"::"r"(src_p));
          asm("movntps %%xmm1,(%0)"::"r"(src_p+4));
          asm("movntps %%xmm2,(%0)"::"r"(src_p+8));
          asm("movntps %%xmm3,(%0)"::"r"(src_p+12));
          asm("movntps %%xmm4,(%0)"::"r"(src_p+16));
          asm("movntps %%xmm5,(%0)"::"r"(src_p+20));
          asm("movntps %%xmm6,(%0)"::"r"(src_p+24));
          asm("movntps %%xmm7,(%0)"::"r"(src_p+28));

          src_p += 32;

          i--;
          keyindex_p++;
        }
    }
  else
    {
      while(i)
        {
          dst_p = dst+((*keyindex_p>>32)<<shift);
          
          for(j=0; j<m; j+=16)
            {
              asm("movaps (%0),%%xmm0"::"r"(dst_p+j));
              asm("movaps (%0),%%xmm1"::"r"(dst_p+j+4));
              asm("movaps (%0),%%xmm2"::"r"(dst_p+j+8));
              asm("movaps (%0),%%xmm3"::"r"(dst_p+j+12));
              
              asm("movntps %%xmm0,(%0)"::"r"(src_p+j));
              asm("movntps %%xmm1,(%0)"::"r"(src_p+j+4));
              asm("movntps %%xmm2,(%0)"::"r"(src_p+j+8));
              asm("movntps %%xmm3,(%0)"::"r"(src_p+j+12));
            }
          
          src_p += j;

          i--;
          keyindex_p++;
        }
    }
#elif SIMD
  register unsigned i = right[id]-left[id],j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift)),*dst_p;
  register unsigned long long *keyindex_p = (unsigned long long*)(src+(right[id]<<shift)-width*2);
  
  while(i)
    {
      dst_p = (unsigned*)(dst+((*keyindex_p>>32)<<shift));
      keyindex_p++;
      i--;
      for(j=0; j<m; j+=4)
        {
          asm("movdqa (%0),%%xmm0"::"r"(dst_p+j));
          asm("movdqa %%xmm0,(%0)"::"r"(src_p+j));
        }
      src_p += j;
    }
#else
  register unsigned i = right[id]-left[id],j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift)),*dst_p;
  register unsigned long long *keyindex_p = (unsigned long long*)(src+(right[id]<<shift)-width*2);
  
  while(i)
    {
      dst_p = (unsigned*)(dst+((*keyindex_p>>32)<<shift));
      keyindex_p++;
      i--;
      for(j=0; j<m+1; j++)
        {
          *(src_p+j) = *(dst_p+j);
        }
      src_p += j;
    }
#endif
}

inline void key_norm_r4(unsigned id)
{
#if NTDQ
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)keyleft[id];
  register unsigned *count_p = count[id];
  
  for(i=left[id]; i<right[id]; i++)
    {
      asm("movaps (%0),%%xmm0"::"r"(src_p));
      asm("movaps (%0),%%xmm1"::"r"(src_p+4));
      asm("movaps (%0),%%xmm2"::"r"(src_p+8));
      asm("movaps (%0),%%xmm3"::"r"(src_p+12));
          
      asm("movaps %%xmm1,(%0)"::"r"(dst_p+4));
      asm("movaps %%xmm2,(%0)"::"r"(dst_p+8));
      asm("movaps %%xmm3,(%0)"::"r"(dst_p+12));
      
      asm("movaps %xmm0,%xmm7");
      asm("mulps %xmm7,%xmm7");
      asm("mulps %xmm1,%xmm1");
      asm("addps %xmm1,%xmm7");
      asm("mulps %xmm2,%xmm2");
      asm("addps %xmm2,%xmm7");
      asm("mulps %xmm3,%xmm3");
      asm("addps %xmm3,%xmm7");
          
      for(j=16; j<m; j+=16)
        {
          asm("movaps (%0),%%xmm1"::"r"(src_p+j));
          asm("movaps (%0),%%xmm2"::"r"(src_p+j+4));
          asm("movaps (%0),%%xmm3"::"r"(src_p+j+8));
          asm("movaps (%0),%%xmm4"::"r"(src_p+j+12));
              
          asm("movntps %%xmm1,(%0)"::"r"(dst_p+j));
          asm("movntps %%xmm2,(%0)"::"r"(dst_p+j+4));
          asm("movntps %%xmm3,(%0)"::"r"(dst_p+j+8));
          asm("movntps %%xmm4,(%0)"::"r"(dst_p+j+12));
              
          asm("mulps %xmm1,%xmm1");
          asm("addps %xmm1,%xmm7");
          asm("mulps %xmm2,%xmm2");
          asm("addps %xmm2,%xmm7");
          asm("mulps %xmm3,%xmm3");
          asm("addps %xmm3,%xmm7");
          asm("mulps %xmm4,%xmm4");
          asm("addps %xmm4,%xmm7");
        }
          
      asm("haddps %xmm7,%xmm7");
      asm("haddps %xmm7,%xmm7");
      asm("movss %xmm7,%xmm0");
          
      asm("movaps %%xmm0,(%0)"::"r"(dst_p));
      
      asm("movss %%xmm0,(%0)"::"r"(keyindex_p));
      count_p[*keyindex_p&0xff]++;
      *(keyindex_p+1) = i;
          
      keyindex_p += 2;
          
      src_p += j;
      dst_p += j;
    }
#elif SIMD
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *count_p = count[id];
  
  for(i=left[id]; i<right[id];)
    {
      asm("movdqa (%0),%%xmm0"::"r"(src_p));
      asm("movdqa %%xmm0,(%0)"::"r"(dst_p));
      asm("mulps %xmm0,%xmm0");
      for(j=4; j<m; j+=4)
        {
          asm("movdqa (%0),%%xmm1"::"r"(src_p+j));
          asm("movdqa %%xmm1,(%0)"::"r"(dst_p+j));
          asm("mulps %xmm1,%xmm1");
          asm("addps %xmm1,%xmm0");
        }
      asm("haddps %xmm0,%xmm0");
      asm("haddps %xmm0,%xmm0");
      asm("movss %%xmm0,(%0)"::"r"(dst_p));
      asm("movss %%xmm0,(%0)"::"r"(keyindex_p));
      count_p[*keyindex_p++&0xff]++;
      *keyindex_p++ = i++;
      
      src_p += j;
      dst_p += j;
    }
#else
  register unsigned i,j;
  register float *src_p = (float*)(src+(left[id]<<shift));
  register float *dst_p = (float*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *count_p = count[id];
  register float norm;
  
  for(i=left[id]; i<right[id]; i++)
    {
      norm = 0;
      for(j=1; j<m+1; j++)
        {
          norm += *(src_p+j)**(src_p+j);
          *(dst_p+j) = *(src_p+j);
        }
      *dst_p = norm;
      *keyindex_p = *(unsigned*)(&norm);
      count_p[*keyindex_p++&0xff]++;
      *keyindex_p++ = i;
      src_p += j;
      dst_p += j;
    }
#endif
}

inline void key_maxv_r4(unsigned id)
{
#if NTDQ
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *count_p = count[id];
      
  if(m == 3)
    {
      for(i=left[id]; i<right[id]; i+=4)
        {
          asm("movaps (%0),%%xmm0"::"r"(src_p));
          asm("movaps (%0),%%xmm1"::"r"(src_p+4));
          asm("movaps (%0),%%xmm2"::"r"(src_p+8));
          asm("movaps (%0),%%xmm3"::"r"(src_p+12));
          
          asm("movaps %xmm0,%xmm4");
          asm("pshufd $0x1b,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("pshufd $0xe1,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("movss %xmm4,%xmm0");
          
          asm("movaps %xmm1,%xmm4");
          asm("pshufd $0x1b,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("pshufd $0xe1,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("movss %xmm4,%xmm1");
          
          asm("movaps %xmm2,%xmm4");
          asm("pshufd $0x1b,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("pshufd $0xe1,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("movss %xmm4,%xmm2");
          
          asm("movaps %xmm3,%xmm4");
          asm("pshufd $0x1b,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("pshufd $0xe1,%xmm4,%xmm5");
          asm("maxps %xmm5,%xmm4");
          asm("movss %xmm4,%xmm3");
          
          asm("movntps %%xmm0,(%0)"::"r"(dst_p));
          asm("movntps %%xmm1,(%0)"::"r"(dst_p+4));
          asm("movntps %%xmm2,(%0)"::"r"(dst_p+8));
          asm("movntps %%xmm3,(%0)"::"r"(dst_p+12));
          
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p));
          count_p[*keyindex_p&0xff]++;
          *(keyindex_p+1) = i;
          asm("movss %%xmm1,(%0)"::"r"(keyindex_p+2));
          count_p[*(keyindex_p+2)&0xff]++;
          *(keyindex_p+3) = i+1;
          asm("movss %%xmm2,(%0)"::"r"(keyindex_p+4));
          count_p[*(keyindex_p+4)&0xff]++;
          *(keyindex_p+5) = i+2;
          asm("movss %%xmm3,(%0)"::"r"(keyindex_p+6));
          count_p[*(keyindex_p+6)&0xff]++;
          *(keyindex_p+7) = i+3;
          
          keyindex_p += 8;
          
          src_p += 16;
          dst_p += 16;
        }
    }
  else
    {
      for(i=left[id]; i<right[id]; i++)
        {
          asm("movaps (%0),%%xmm0"::"r"(src_p));
          asm("movaps (%0),%%xmm1"::"r"(src_p+4));
          asm("movaps (%0),%%xmm2"::"r"(src_p+8));
          asm("movaps (%0),%%xmm3"::"r"(src_p+16));
          
          asm("movaps %%xmm1,(%0)"::"r"(dst_p+4));
          asm("movaps %%xmm2,(%0)"::"r"(dst_p+8));
          asm("movaps %%xmm3,(%0)"::"r"(dst_p+12));
          
          asm("movaps %xmm0,%xmm7");
          asm("maxps %xmm1,%xmm7");
          asm("maxps %xmm2,%xmm7");
          asm("maxps %xmm3,%xmm7");
          
          for(j=16; j<m; j+=16)
            {
              asm("movaps (%0),%%xmm1"::"r"(src_p+j));
              asm("movaps (%0),%%xmm2"::"r"(src_p+j+4));
              asm("movaps (%0),%%xmm3"::"r"(src_p+j+8));
              asm("movaps (%0),%%xmm4"::"r"(src_p+j+12));
              
              asm("maxps %xmm1,%xmm7");
              asm("maxps %xmm2,%xmm7");
              asm("maxps %xmm3,%xmm7");
              asm("maxps %xmm4,%xmm7");

              asm("movntps %%xmm1,(%0)"::"r"(dst_p+j));
              asm("movntps %%xmm2,(%0)"::"r"(dst_p+j+4));
              asm("movntps %%xmm3,(%0)"::"r"(dst_p+j+8));
              asm("movntps %%xmm4,(%0)"::"r"(dst_p+j+12));
            }
          
          asm("pshufd $0x1b,%xmm7,%xmm6");
          asm("maxps %xmm6,%xmm7");
          asm("pshufd $0xe1,%xmm7,%xmm6");
          asm("maxps %xmm6,%xmm7");
          
          asm("movss %xmm7,%xmm0");
          asm("movaps %%xmm0,(%0)"::"r"(dst_p));
          
          asm("movss %%xmm0,(%0)"::"r"(keyindex_p));
          count_p[*keyindex_p&0xff]++;
          *(keyindex_p+1) = i;
          
          keyindex_p += 2;
          
          src_p += j;
          dst_p += j;
        }
    }
#elif SIMD
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *count_p = count[id];
  
  for(i=left[id]; i<right[id];)
    {
      asm("movdqa (%0),%%xmm0"::"r"(src_p));
      asm("movdqa %%xmm0,(%0)"::"r"(dst_p));
      for(j=4; j<m; j+=4)
        {
          asm("movdqa (%0),%%xmm1"::"r"(src_p+j));
          asm("movdqa %%xmm1,(%0)"::"r"(dst_p+j));
          asm("maxps %xmm1,%xmm0");
        }
      asm("movdqa %xmm0,%xmm1");
      asm("shufps $0x1b,%xmm0,%xmm0");
      asm("maxps %xmm1,%xmm0");
      asm("movdqa %xmm0,%xmm1");
      asm("shufps $0xe1,%xmm0,%xmm0");
      asm("maxps %xmm1,%xmm0");
      asm("movss %%xmm0,(%0)"::"r"(dst_p));
      asm("movss %%xmm0,(%0)"::"r"(keyindex_p));
      count_p[*keyindex_p++&0xff]++;
      *keyindex_p++ = i++;
      src_p += j;
      dst_p += j;
    }
#else
  register unsigned i,j;
  register unsigned *src_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned *dst_p = (unsigned*)(dst+(left[id]<<shift));
  register unsigned *keyindex_p = (unsigned*)(src+(left[id]<<shift));
  register unsigned maxv;
  register unsigned *count_p = count[id];
  
  for(i=left[id]; i<right[id]; i++)
    {
      maxv = *(src_p+1);
      *(dst_p+1) = *(src_p+1);
      for(j=2; j<m+1; j++)
        {
          if(*(src_p+j) > maxv)
            {
              maxv = *(src_p+j);
            }
          *(dst_p+j) = *(src_p+j);
        }
      *dst_p = maxv;
      *keyindex_p = maxv;
      count_p[*keyindex_p++&0xff]++;
      *keyindex_p++ = i;
      src_p += j;
      dst_p += j;
    }
#endif
}

inline void cnt1_r4(unsigned id)
{
}

inline void rdx1_r4(unsigned id)
{
  register unsigned i = width,offset,tmp;
  register unsigned long long *keyindex_p = keyleft[id];
  register unsigned *count_p = count[id];
  
  while(i--)
    {
      offset = count_p[*keyindex_p&0xff]++;
      tmp = offset/width;
      *(keyright[tmp]+offset-tmp*width) = *keyindex_p++;
    }
}

inline void cnt2_r4(unsigned id)
{
  register unsigned i;
  register unsigned long long *keyindex_p = keyright[id];
  register unsigned *count_p = count[id];
  
  for(i=0; i<1<<8; i++)
    {
      count_p[i] = 0;
    }
  i = width;
  while(i--)
    {
      count_p[(*keyindex_p>>8)&0xff]++;
      keyindex_p++;
    }
}

inline void rdx2_r4(unsigned id)
{
  register unsigned i = width,offset,tmp;
  register unsigned long long *keyindex_p = keyright[id];
  register unsigned *count_p = count[id];
  
  while(i--)
    {
      offset = count_p[(*keyindex_p>>8)&0xff]++;
      tmp = offset/width;
      *(keyleft[tmp]+offset-tmp*width) = *keyindex_p++;
    }
}

inline void cnt3_r4(unsigned id)
{
  register unsigned i;
  register unsigned long long *keyindex_p = keyleft[id];
  register unsigned *count_p = count[id];
  
  for(i=0; i<1<<8; i++)
    {
      count_p[i] = 0;
    }
  i = width;
  while(i--)
    {
      count_p[(*keyindex_p>>16)&0xff]++;
      keyindex_p++;
    }
}

inline void rdx3_r4(unsigned id)
{
  register unsigned i = width,offset,tmp;
  register unsigned long long *keyindex_p = keyleft[id];
  register unsigned *count_p = count[id];
  
  while(i--)
    {
      offset = count_p[(*keyindex_p>>16)&0xff]++;
      tmp = offset/width;
      *(keyright[tmp]+offset-tmp*width) = *keyindex_p++;
    }
}

inline void cnt4_r4(unsigned id)
{
  register unsigned i;
  register unsigned long long *keyindex_p = keyright[id];
  register unsigned *count_p = count[id];
  
  for(i=0; i<1<<8; i++)
    {
      count_p[i] = 0;
    }
  i = width;
  while(i--)
    {
      count_p[(*keyindex_p>>24)&0xff]++;
      keyindex_p++;
    }
}

inline void rdx4_r4(unsigned id)
{
  register unsigned i = width,offset,tmp;
  register unsigned long long *keyindex_p = keyright[id];
  register unsigned *count_p = count[id];
  
  while(i--)
    {
      offset = count_p[(*keyindex_p>>24)&0xff]++;
      tmp = offset/width;
      *(keyleft[tmp]+offset-tmp*width) = *keyindex_p++;
    }
}

inline void move_r4(unsigned id)
{
#if NTDQ
  register unsigned i = width,j;
  register unsigned *src_p = (unsigned*)(src+((right[id]-1)<<shift)),*dst_p;
  register unsigned long long *keyindex_p = (unsigned long long*)(src+(left[id]<<shift)+(width-1)*2);
  
  if(m == 3)
    {
      while(i)
        {
          dst_p = dst+((*keyindex_p>>32)<<shift);
          
          asm("movaps (%0),%%xmm0"::"r"(dst_p));
          
          dst_p = dst+((*(keyindex_p-1)>>32)<<shift);
          
          asm("movaps (%0),%%xmm1"::"r"(dst_p));
          
          dst_p = dst+((*(keyindex_p-2)>>32)<<shift);
          
          asm("movaps (%0),%%xmm2"::"r"(dst_p));
          
          dst_p = dst+((*(keyindex_p-3)>>32)<<shift);
          
          asm("movaps (%0),%%xmm3"::"r"(dst_p));
          
          asm("movntps %%xmm0,(%0)"::"r"(src_p));
          asm("movntps %%xmm1,(%0)"::"r"(src_p-4));
          asm("movntps %%xmm2,(%0)"::"r"(src_p-8));
          asm("movntps %%xmm3,(%0)"::"r"(src_p-12));

          src_p -= 16;

          i -= 4;
          keyindex_p -= 4;
        }
    }
  else
    {
      while(i)
        {
          dst_p = dst+((*keyindex_p>>32)<<shift);
          
          for(j=0; j<m; j+=16)
            {
              asm("movaps (%0),%%xmm0"::"r"(dst_p+j));
              asm("movaps (%0),%%xmm1"::"r"(dst_p+j+4));
              asm("movaps (%0),%%xmm2"::"r"(dst_p+j+8));
              asm("movaps (%0),%%xmm3"::"r"(dst_p+j+12));
              
              asm("movntps %%xmm0,(%0)"::"r"(src_p+j));
              asm("movntps %%xmm1,(%0)"::"r"(src_p+j+4));
              asm("movntps %%xmm2,(%0)"::"r"(src_p+j+8));
              asm("movntps %%xmm3,(%0)"::"r"(src_p+j+12));
            }
          
          src_p -= j;

          i--;
          keyindex_p--;
        }
    }
#elif SIMD
  register unsigned i = right[id]-left[id],j;
  register unsigned *src_p = (unsigned*)(src+((right[id]-1)<<shift)),*dst_p;
  register unsigned long long *keyindex_p = (unsigned long long*)(src+(left[id]<<shift)+(width-1)*2);
  
  while(i)
    {
      dst_p = (unsigned*)(dst+((*keyindex_p>>32)<<shift));
      keyindex_p--;
      i--;
      for(j=0; j<m; j+=4)
        {
          asm("movdqa (%0),%%xmm0"::"r"(dst_p+j));
          asm("movdqa %%xmm0,(%0)"::"r"(src_p+j));
        }
      src_p -= j;
    }
#else
  register unsigned i = right[id]-left[id],j;
  register unsigned *src_p = (unsigned*)(src+((right[id]-1)<<shift)),*dst_p;
  register unsigned long long *keyindex_p = (unsigned long long*)(src+(left[id]<<shift)+(width-1)*2);
  
  while(i)
    {
      dst_p = (unsigned*)(dst+((*keyindex_p>>32)<<shift));
      keyindex_p--;
      i--;
      for(j=0; j<m+1; j++)
        {
          *(src_p+j) = *(dst_p+j);
        }
      src_p -= j;
    }
#endif
}

inline void sum1_8(unsigned id)
{
  register unsigned i,j,tmp1,tmp2,left,right;
  register unsigned *count_p;

  if(nth == 8)
    {
      j = 32*id;
      tmp2 = count[0][j];
      count[0][j]=0;
      tmp1 = count[1][j];
      count[1][j] = count[0][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[2][j];
      count[2][j] = count[1][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[3][j];
      count[3][j] = count[2][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[4][j];
      count[4][j] = count[3][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[5][j];
      count[5][j] = count[4][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[6][j];
      count[6][j] = count[5][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[7][j];
      count[7][j] = count[6][j]+tmp2;
      tmp2 = tmp1;
      for(i=j+1; i<j+32; i++)
        {
          tmp1 = count[0][i];
          count[0][i] = count[nth-1][i-1]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[1][i];
          count[1][i] = count[0][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[2][i];
          count[2][i] = count[1][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[3][i];
          count[3][i] = count[2][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[4][i];
          count[4][i] = count[3][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[5][i];
          count[5][i] = count[4][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[6][i];
          count[6][i] = count[5][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[7][i];
          count[7][i] = count[6][i]+tmp2;
          tmp2 = tmp1;
        }
      hist[id+1] = count[7][i-1]+tmp2;
    }
  else
    {
      left = (1<<8)/nth*id;
      if(id == nth-1)
        {
          right = 1<<8;
        }
      else
        {
          right = (1<<8)/nth*(id+1);
        }
      tmp1 = count[0][left];
      count[0][left]=0;
      tmp2 = tmp1;
      for(j=1; j<nth; j++)
        {
          tmp1 = count[j][left];
          count[j][left] = count[j-1][left]+tmp2;
          tmp2 = tmp1;
        }
      for(i=left+1; i<right; i++)
        {
          tmp1 = count[0][i];
          count[0][i] = count[nth-1][i-1]+tmp2;
          tmp2 = tmp1;
          for(j=1; j<nth; j++)
            {
              tmp1 = count[j][i];
              count[j][i] = count[j-1][i]+tmp2;
              tmp2 = tmp1;
            }
        }
      hist[id+1] = count[nth-1][right-1]+tmp2;
    }
}

inline void sum2_8(unsigned id)
{
  register unsigned i;
  
  if(nth == 8)
    {
      if(id == 0)
        {
          hist[0] = 0;
          hist[2] += hist[1];
          hist[3] += hist[2];
          hist[4] += hist[3];
          hist[5] += hist[4];
          hist[6] += hist[5];
          hist[7] += hist[6];
        }
    }
  else
    {
      if(id == 0)
        {
          hist[0] = 0;
          for(i=1; i<nth; i++)
            {
              hist[i] += hist[i-1];
            }
        }
    }
}

inline void sum3_8(unsigned id)
{
  register unsigned i,j,tmp1,tmp2,left,right;
  register unsigned *count_p;
  
  if(nth == 8)
    {
      tmp1 = (1<<8)/nth*id;
      tmp2 = hist[id];

      count_p = count[0]+tmp1;
      i = 32;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[1]+tmp1;
      i = 32;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[2]+tmp1;
      i = 32;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[3]+tmp1;
      i = 32;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[4]+tmp1;
      i = 32;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[5]+tmp1;
      i = 32;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[6]+tmp1;
      i = 32;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[7]+tmp1;
      i = 32;
      while(i--)
        {
          *count_p++ += tmp2;
        }
    }
  else
    {
      left = (1<<8)/nth*id;
      if(id == nth-1)
        {
          right = 1<<8;
        }
      else
        {
          right = (1<<8)/nth*(id+1);
        }
      for(j=0; j<nth; j++)
        {
          for(i=left; i<right; i++)
            {
              count[j][i] += hist[id];
            }
        }
    }
}

inline void sum1_10(unsigned id)
{
  register unsigned i,j,tmp1,tmp2,left,right;
  register unsigned *count_p;
  
  if(nth == 8)
    {
      j = 128*id;
      tmp2 = count[0][j];
      count[0][j]=0;
      tmp1 = count[1][j];
      count[1][j] = count[0][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[2][j];
      count[2][j] = count[1][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[3][j];
      count[3][j] = count[2][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[4][j];
      count[4][j] = count[3][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[5][j];
      count[5][j] = count[4][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[6][j];
      count[6][j] = count[5][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[7][j];
      count[7][j] = count[6][j]+tmp2;
      tmp2 = tmp1;
      for(i=j+1; i<j+128; i++)
        {
          tmp1 = count[0][i];
          count[0][i] = count[nth-1][i-1]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[1][i];
          count[1][i] = count[0][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[2][i];
          count[2][i] = count[1][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[3][i];
          count[3][i] = count[2][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[4][i];
          count[4][i] = count[3][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[5][i];
          count[5][i] = count[4][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[6][i];
          count[6][i] = count[5][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[7][i];
          count[7][i] = count[6][i]+tmp2;
          tmp2 = tmp1;
        }
      hist[id+1] = count[7][i-1]+tmp2;
    }
  else
    {
      left = (1<<10)/nth*id;
      if(id == nth-1)
        {
          right = 1<<10;
        }
      else
        {
          right = (1<<10)/nth*(id+1);
        }
      tmp1 = count[0][left];
      count[0][left]=0;
      tmp2 = tmp1;
      for(j=1; j<nth; j++)
        {
          tmp1 = count[j][left];
          count[j][left] = count[j-1][left]+tmp2;
          tmp2 = tmp1;
        }
      for(i=left+1; i<right; i++)
        {
          tmp1 = count[0][i];
          count[0][i] = count[nth-1][i-1]+tmp2;
          tmp2 = tmp1;
          for(j=1; j<nth; j++)
            {
              tmp1 = count[j][i];
              count[j][i] = count[j-1][i]+tmp2;
              tmp2 = tmp1;
            }
        }
      hist[id+1] = count[nth-1][right-1]+tmp2;
    }
}

inline void sum2_10(unsigned id)
{
  register unsigned i;
  
  if(nth == 8)
    {
      if(id == 0)
        {
          hist[0] = 0;
          hist[2] += hist[1];
          hist[3] += hist[2];
          hist[4] += hist[3];
          hist[5] += hist[4];
          hist[6] += hist[5];
          hist[7] += hist[6];
        }
    }
  else
    {
      if(id == 0)
        {
          hist[0] = 0;
          for(i=1; i<nth; i++)
            {
              hist[i] += hist[i-1];
            }
        }
    }
}

inline void sum3_10(unsigned id)
{
  register unsigned i,j,tmp1,tmp2,left,right;
  register unsigned *count_p;
  
  if(nth == 8)
    {
      tmp1 = (1<<10)/nth*id;
      tmp2 = hist[id];

      count_p = count[0]+tmp1;
      i = 128;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[1]+tmp1;
      i = 128;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[2]+tmp1;
      i = 128;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[3]+tmp1;
      i = 128;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[4]+tmp1;
      i = 128;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[5]+tmp1;
      i = 128;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[6]+tmp1;
      i = 128;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[7]+tmp1;
      i = 128;
      while(i--)
        {
          *count_p++ += tmp2;
        }
    }
  else
    {
      left = (1<<10)/nth*id;
      if(id == nth-1)
        {
          right = 1<<10;
        }
      else
        {
          right = (1<<10)/nth*(id+1);
        }
      for(j=0; j<nth; j++)
        {
          for(i=left; i<right; i++)
            {
              count[j][i] += hist[id];
            }
        }
    }
}

inline void sum1_11(unsigned id)
{
  register unsigned i,j,tmp1,tmp2,left,right;
  register unsigned *count_p;

  if(nth == 8)
    {
      j = 256*id;
      tmp2 = count[0][j];
      count[0][j]=0;
      tmp1 = count[1][j];
      count[1][j] = count[0][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[2][j];
      count[2][j] = count[1][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[3][j];
      count[3][j] = count[2][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[4][j];
      count[4][j] = count[3][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[5][j];
      count[5][j] = count[4][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[6][j];
      count[6][j] = count[5][j]+tmp2;
      tmp2 = tmp1;
      tmp1 = count[7][j];
      count[7][j] = count[6][j]+tmp2;
      tmp2 = tmp1;
      for(i=j+1; i<j+256; i++)
        {
          tmp1 = count[0][i];
          count[0][i] = count[nth-1][i-1]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[1][i];
          count[1][i] = count[0][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[2][i];
          count[2][i] = count[1][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[3][i];
          count[3][i] = count[2][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[4][i];
          count[4][i] = count[3][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[5][i];
          count[5][i] = count[4][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[6][i];
          count[6][i] = count[5][i]+tmp2;
          tmp2 = tmp1;
          tmp1 = count[7][i];
          count[7][i] = count[6][i]+tmp2;
          tmp2 = tmp1;
        }
      hist[id+1] = count[7][i-1]+tmp2;
    }
  else
    {
      left = (1<<11)/nth*id;
      if(id == nth-1)
        {
          right = 1<<11;
        }
      else
        {
          right = (1<<11)/nth*(id+1);
        }
      tmp1 = count[0][left];
      count[0][left]=0;
      tmp2 = tmp1;
      for(j=1; j<nth; j++)
        {
          tmp1 = count[j][left];
          count[j][left] = count[j-1][left]+tmp2;
          tmp2 = tmp1;
        }
      for(i=left+1; i<right; i++)
        {
          tmp1 = count[0][i];
          count[0][i] = count[nth-1][i-1]+tmp2;
          tmp2 = tmp1;
          for(j=1; j<nth; j++)
            {
              tmp1 = count[j][i];
              count[j][i] = count[j-1][i]+tmp2;
              tmp2 = tmp1;
            }
        }
      hist[id+1] = count[nth-1][right-1]+tmp2;
    }
}

inline void sum2_11(unsigned id)
{
  register unsigned i;
  
  if(nth == 8)
    {
      if(id == 0)
        {
          hist[0] = 0;
          hist[2] += hist[1];
          hist[3] += hist[2];
          hist[4] += hist[3];
          hist[5] += hist[4];
          hist[6] += hist[5];
          hist[7] += hist[6];
        }
    }
  else
    {
      if(id == 0)
        {
          hist[0] = 0;
          for(i=1; i<nth; i++)
            {
              hist[i] += hist[i-1];
            }
        }
    }
}

inline void sum3_11(unsigned id)
{
  register unsigned i,j,tmp1,tmp2,left,right;
  register unsigned *count_p;
  
  if(nth == 8)
    {
      tmp1 = (1<<11)/nth*id;
      tmp2 = hist[id];

      count_p = count[0]+tmp1;
      i = 256;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[1]+tmp1;
      i = 256;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[2]+tmp1;
      i = 256;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[3]+tmp1;
      i = 256;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[4]+tmp1;
      i = 256;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[5]+tmp1;
      i = 256;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[6]+tmp1;
      i = 256;
      while(i--)
        {
          *count_p++ += tmp2;
        }
      count_p = count[7]+tmp1;
      i = 256;
      while(i--)
        {
          *count_p++ += tmp2;
        }
    }
  else
    {
      left = (1<<11)/nth*id;
      if(id == nth-1)
        {
          right = 1<<11;
        }
      else
        {
          right = (1<<11)/nth*(id+1);
        }
      for(j=0; j<nth; j++)
        {
          for(i=left; i<right; i++)
            {
              count[j][i] += hist[id];
            }
        }
    }
}

