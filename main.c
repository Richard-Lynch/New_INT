/*
    Calculation of n digits of Pi.
    Created by Richard Lynch January 30th 2017.

    Based on program written by: 
    Fabrice Bellard on January 8, 1997.
    "Computation of the n'th decimal digit of \pi with very little memory"
    The algorithm was modified to get a running time of O(n^2) instead of O(n^3log(n)^3).

    Based on method described by:
    Simon Plouffe in "On the Computation of the n'th decimal digit of various 
    transcendental numbers" (November 1996). 

    * This program uses mostly integer arithmetic. It may be slow on some
    * hardwares where integer multiplications and divisons must be done
    * by software. We have supposed that 'int' has a size of 32 bits. If
    * your compiler supports 'long long' integers of 64 bits, you may use
    * the integer version of 'mul_mod' (see HAS_LONG_LONG).  
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

/* uncomment the following line to use 'long long' integers */
#define HAS_LONG_LONG

#ifdef HAS_LONG_LONG
#define mul_mod(a, b, m) (((long long)(a) * (long long)(b)) % (m))
#else
#define mul_mod(a, b, m) fmod((double)a *(double)b, m)
#endif

int NUM_THREADS = 6.00L;
int *results;
int curr_x = 0;
int MAX_INDEX;

pthread_mutex_t sum_lock;
pthread_mutex_t curr_x_lock;

/* return the inverse of x mod y - FOUND ONLINE*/
int inv_mod(int x, int y)
{
    int q, u, v, a, c, t;

    u = x;
    v = y;
    c = 1;
    a = 0;
    do
    {
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
    while (1)
    {
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

    r = (int)(sqrt(n));
    for (i = 3; i <= r; i += 2)
        if ((n % i) == 0)
            return 0;
    return 1;
}

/* return the prime number immediatly after n - FOUND ONLINE*/
int next_prime(int n)
{
    do
    {
        n++;
    } while (!is_prime(n));
    return n;
}

// Thread which calcualtes the next 9 digits of pi from the current staring pos
void *calc(void *thread)
{
    int av, a, vmax, N, n, num, den, k, kq, kq2, t, v, s, i;
    double sum;
    int this_thread = *(int *)thread;

    bool loop = false;

    //iterate
    pthread_mutex_lock(&curr_x_lock);
    int index = curr_x;
    curr_x++;
    pthread_mutex_unlock(&curr_x_lock);

    // check if done
    if (index < MAX_INDEX)
    {
        loop = true;
    }
    else
    {
        loop = false;
    }

    while (loop)
    {
        printf("Thread %d entering loop at index %d\n", this_thread, index);

        n = ((index + 1) * 9) - 8;  //find the digit we want to calculate from
        N = (int)((n + 20) * log(10) / log(2));

        sum = 0;

        //utterly mad maths to get calc the digits - see referanced documents
        for (a = 3; a <= (2 * N); a = next_prime(a))
        {

            vmax = (int)(log(2 * N) / log(a));
            av = 1;
            for (i = 0; i < vmax; i++)
                av = av * a;

            s = 0;
            num = 1;
            den = 1;
            v = 0;
            kq = 1;
            kq2 = 1;

            for (k = 1; k <= N; k++)
            {
                t = k;
                if (kq >= a)
                {
                    do
                    {
                        t = t / a;
                        v--;
                    } while ((t % a) == 0);
                    kq = 0;
                }
                kq++;
                num = mul_mod(num, t, av);

                t = (2 * k - 1);
                if (kq2 >= a)
                {
                    if (kq2 == a)
                    {
                        do
                        {
                            t = t / a;
                            v++;
                        } while ((t % a) == 0);
                    }
                    kq2 -= a;
                }
                den = mul_mod(den, t, av);
                kq2 += 2;

                if (v > 0)
                {
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
            sum = fmod(sum + (double)s / (double)av, 1.0);
        }
        // output the calculate digits
        int curr = (sum * 1e9);
        printf("Decimal digits of pi at position %d from thread  %d: %09d\n", n, this_thread, curr);

        // add the value to the results array
        pthread_mutex_lock(&sum_lock);
        results[index] = curr;
        pthread_mutex_unlock(&sum_lock);

        //iterate to the next index
        pthread_mutex_lock(&curr_x_lock);
        index = curr_x;
        curr_x++;
        pthread_mutex_unlock(&curr_x_lock);

        // check if index is valid
        if (index < MAX_INDEX)
        {
            loop = true;
        }
        else
        {
            loop = false;
        }
    }
    printf("Thread %d Done\n", this_thread);
    // on end, result void* to NULL ( could be number of loops )
    void *vo = NULL;
    pthread_exit(vo);
}

int main(int argc, char *argv[])
{
    // read args
    int n_digits;
    if (argc < 2 || (n_digits = atoi(argv[1])) <= 0)
    { //convert the first arg to an int and set n
        //if there is not arg entered
        printf("This program computes n digits of pi\n"
               "\"usage: main n\" , where n is the number of digits after the decimal point you want\n");
        exit(1);
    }

    // init vals
    int n_ints = n_digits / 9;
    if (n_digits % 9 != 0)
    {
        n_ints++;
    }
    MAX_INDEX = n_ints;
    results = malloc(sizeof(int) * n_ints); //Stores results

    // create local vars
    pthread_t threads[NUM_THREADS];
    int rc, t;
    void *v0 = NULL;
    int thread_index[NUM_THREADS];

    // start clock
    clock_t start = clock();
    printf("Creating top thread.\n");
    // create threads
    for (t = 0; t < NUM_THREADS; t++)
    {
        thread_index[t] = t;
        rc = pthread_create(&threads[t], NULL, calc, (void *)(&thread_index[t]));
        if (rc)
        {
            printf("ERROR - return code from thread %d pthread_create: %d\n", t, rc);
            exit(-1);
        }
        printf("Finished creating thread: %d\n", t);
    }

    // join the threads
    for (t = 0; t < NUM_THREADS; t++)
    {
        rc = pthread_join(threads[t], v0);
        if (rc)
        {
            printf("ERROR - return code from pthread_join: %d\n", rc);
            exit(-1);
        }
        else
        {
            printf("Joined thread: %d\n", t);
        }
    }
    
    printf("Finished Joining threads\n");
    // stop the cock
    clock_t tdiff = clock() - start;

    // output result
    printf("---RESULTS----\n");
    printf("Fucntion: “Bailey-Borwein-Plouffe” (BBP)\n");
    char pi_string[] = "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679";
    printf("Actual: %s\n", pi_string);
    // output time
    long double msec = tdiff * 1000.00L / CLOCKS_PER_SEC;
    printf("Time taken %d seconds %d milliseconds \n", (int)msec / 1000, (int)msec % 1000);
    printf("Time taken %.10Lf Miliseconds\n", msec);

    pthread_mutex_lock(&sum_lock);
    // buffer and print pi
    char buffer[10];
    char *BigBuffer = malloc(sizeof(char) * 1000);
    printf("Result: 3.");
    for (int i = 0; i < n_ints - 1; i++)
    {
        sprintf(buffer, "%d", results[i]);
        sprintf(BigBuffer, "%s%d", BigBuffer, results[i]);
        printf("%.9s", buffer);
    }
    sprintf(buffer, "%d", results[n_ints - 1]);
    int remain = (n_digits % 9);
    sprintf(BigBuffer, "%s%.*s", BigBuffer, remain, buffer);
    printf("%.*s\n", remain, buffer);
    printf("\nBig Buffer\nResult: 3.%s\n\n", BigBuffer);

    // find any digit of pi within range
    char digit_index[5];
    while (1)
    {
        printf("Please enter the digit of pi you would like:");
        gets(digit_index);
        printf("\n");
        if (digit_index[0] == 'e' && digit_index[1] == 'n' && digit_index[2] == 'd')
        {
            break;
        }
        else
        {
            printf("The %dth digit of pi is: %c\n", atoi(&digit_index[0]), BigBuffer[atoi(&digit_index[0]) - 1]);
        }
    }
    free(BigBuffer);
    pthread_mutex_unlock(&sum_lock);

    // destroy mutex
    pthread_mutex_destroy(&sum_lock);
    pthread_mutex_destroy(&curr_x_lock);

    // done
    printf("Goodbye\n\n");

    return 0;
}