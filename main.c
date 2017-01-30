/*
     * Computation of the n'th decimal digit of \pi with very little memory.
     * Written by Fabrice Bellard on January 8, 1997.
     * 
     * We use a slightly modified version of the method described by Simon
     * Plouffe in "On the Computation of the n'th decimal digit of various
     * transcendental numbers" (November 1996). We have modified the algorithm
     * to get a running time of O(n^2) instead of O(n^3log(n)^3).
     * 
     * This program uses mostly integer arithmetic. It may be slow on some
     * hardwares where integer multiplications and divisons must be done
     * by software. We have supposed that 'int' has a size of 32 bits. If
     * your compiler supports 'long long' integers of 64 bits, you may use
     * the integer version of 'mul_mod' (see HAS_LONG_LONG).  
     */

    #include <stdlib.h>
    #include <stdio.h>
    #include <math.h>

/* uncomment the following line to use 'long long' integers */
#define HAS_LONG_LONG

#ifdef HAS_LONG_LONG
#define mul_mod(a,b,m) (( (long long) (a) * (long long) (b) ) % (m))
#else
#define mul_mod(a,b,m) fmod( (double) a * (double) b, m)
#endif


// -------
#include <pthread.h>
#include <time.h>

// int NUM_DIGITS = 9;
int NUM_THREADS = 6.00L;
long double reS = 3.14159265359L;
int* results;
int curr_n =1;

pthread_mutex_t num_threads_lock;
pthread_mutex_t sum_lock;
pthread_mutex_t curr_x_lock;

long double curr_x = 1.00L;
long double sum = 0;


// ----------

/* return the inverse of x mod y */
int inv_mod(int x, int y)
{
    int q, u, v, a, c, t;

    u = x;
    v = y;
    c = 1;
    a = 0;
    do {
    q = v / u;

    t = c;
    c = a - q * c;
    a = t;

    t = u;
    u = v - q * u;
    v = t;
    } while (u != 0);
    a = a % y;
    if (a < 0)
    a = y + a;
    return a;
}

/* return (a^b) mod m - FOUND ONLINE*/ 
int pow_mod(int a, int b, int mod)
{
    int result, aa;

    result = 1;
    aa = a;
    while (1) {
    if (b & 1)
        result = mul_mod(result, aa, mod);
    b = b >> 1;
    if (b == 0)
        break;
    aa = mul_mod(aa, aa, mod);
    }
    return result;
}

/* return true if n is prime - FOUND ONLINE*/
int is_prime(int n)
{
    int r, i;
    if ((n % 2) == 0)
    return 0;

    r = (int) (sqrt(n));
    for (i = 3; i <= r; i += 2)
    if ((n % i) == 0)
        return 0;
    return 1;
}

/* return the prime number immediatly after n - FOUND ONLINE*/
int next_prime(int n)
{
    do {
    n++;
    } while (!is_prime(n));
    return n;
}

// 
void* calc(void* n_pos){
    int av, a, vmax, N, n, num, den, k, kq, kq2, t, v, s, i;
    double sum;

    n = *(int*)n_pos;
    N = (int) ((n + 20) * log(10) / log(2));

    sum = 0;

    for (a = 3; a <= (2 * N); a = next_prime(a)) {

    vmax = (int) (log(2 * N) / log(a));
    av = 1;
    for (i = 0; i < vmax; i++)
        av = av * a;

    s = 0;
    num = 1;
    den = 1;
    v = 0;
    kq = 1;
    kq2 = 1;

    for (k = 1; k <= N; k++) {

        t = k;
        if (kq >= a) {
        do {
            t = t / a;
            v--;
        } while ((t % a) == 0);
        kq = 0;
        }
        kq++;
        num = mul_mod(num, t, av);

        t = (2 * k - 1);
        if (kq2 >= a) {
        if (kq2 == a) {
            do {
            t = t / a;
            v++;
            } while ((t % a) == 0);
        }
        kq2 -= a;
        }
        den = mul_mod(den, t, av);
        kq2 += 2;

        if (v > 0) {
        t = inv_mod(den, av);
        t = mul_mod(t, num, av);
        t = mul_mod(t, k, av);
        for (i = v; i < vmax; i++)
            t = mul_mod(t, a, av);
        s += t;
        if (s >= av)
            s -= av;
        }

    }

    t = pow_mod(10, n - 1, av);
    s = mul_mod(s, t, av);
    sum = fmod(sum + (double) s / (double) av, 1.0);
    }
    //    (int) (sum * 1e9));
    int* curr = malloc(sizeof(int));
    *curr = (sum * 1e9);
    printf("Decimal digits of pi at position %d: %09d\n", n, *curr);
    return (void*)curr;
    // pthread_exit((void*)curr); 
    
}

int main(int argc, char *argv[])
{
    int n_digits = atoi(argv[1]);
    int n_ints = n_digits/9;
    if(n_digits % 9 != 0){
        n_ints++; 
    }
    results = malloc(sizeof(int)*n_ints);

    

    // if (argc < 2 || (n = atoi(argv[1])) <= 0) { //convert the first arg to an int and set n
    //     //if there is not arg entered
    // printf("This program computes the n'th decimal digit of \\pi\n"
    //        "usage: pi n , where n is the digit you want\n");
    // exit(1);
    // }



    calc((void*)(&curr_n));


    return 0;
}