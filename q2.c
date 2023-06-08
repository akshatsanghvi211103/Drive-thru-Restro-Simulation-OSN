#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

typedef struct Customer
{
    int entryTime;
    int numPizzas;
    int *pizzaIDs;
} Customer;
typedef struct Pizza
{
    int prepTime;
    int numIngredients;
    int *ingredients;
} Pizza;
typedef struct Chef
{
    int entryTime;
    int exitTime;
    int prepareTime; // without the oven ka extra time, and set by the customer thread
    int numPizzas;   // from the pizza order from the customers
    int assignedCustomer;
    int *pizzaIDs; // from the pizza order from the customers

    pthread_cond_t chefSignal;
    pthread_mutex_t busyCheckLock;
} Chef;

int n, m, num, c, o, s; // num is the number of ingredients!
int *ingredients;
Chef *chefs;
Pizza *pizzas;
Customer *customers;

int numCustomers;

sem_t ovens;
pthread_mutex_t numCustomersLock;

void *customerRoutine(void *args)
{
    int index = *(int *)args;
    int entryTime = customers[index].entryTime;
    int totTime = 0;
    sleep(entryTime);
    // Ingredient Lock???
    for (int i = 0; i < customers[index].numPizzas; i++)
    {
        totTime += pizzas[customers[index].pizzaIDs[i]].prepTime;
        for (int j = 0; j < pizzas[customers[index].pizzaIDs[i]].numIngredients; j++)
        {
            if (ingredients[pizzas[customers[index].pizzaIDs[i]].ingredients[j]] < 1)
            {
                printf("INGREDIENTS NOT ENUFF for customer %d\n", index);
                return 0;
            }
        }
    }
    for (int i = 0; i < n; i++)
    {
        int free = 0;
        if (pthread_mutex_trylock(&chefs[i].busyCheckLock) == 0)
        {
            if (totTime + s + entryTime <= chefs[i].exitTime)
            {
                printf("Customer %d's food is getting prepared\n", index);
                // putting the ingredients
                chefs[i].prepareTime = totTime;
                chefs[i].numPizzas = customers[index].numPizzas;
                chefs[i].pizzaIDs = customers[index].pizzaIDs;
                chefs[i].assignedCustomer = index;
                for (int i = 0; i < customers[index].numPizzas; i++)
                {
                    for (int j = 0; j < pizzas[customers[index].pizzaIDs[i]].numIngredients; j++)
                    {
                        ingredients[pizzas[customers[index].pizzaIDs[i]].ingredients[j]]--;
                    }
                }
                pthread_cond_signal(&chefs[i].chefSignal); // where to do sleep(s);????????????????????????????????????
                return 0;
            }
            pthread_mutex_unlock(&chefs[i].busyCheckLock);
        }
    }
    printf("Customer %d cudn't get the food, REJECTED\n", index);
}
void *chefRoutine(void *args)
{
    int timeUsed = 0;
    int index = *(int *)args;
    int timeLeft = chefs[index].exitTime - chefs[index].entryTime;
    sleep(chefs[index].entryTime);
    // assuming the customer gave a signal to this chef, only then...use a for loop to decide which chef will be assigned
    // check the condition whether this chef can prepare the pizzas or not before itself
    // also check if the chef is busy or not...dont forget to use a lock while checking this
    // repeat the routine after
    struct timespec ts;
    pthread_mutex_t fakeMutex;
repeat:
    timeUsed = time(0);
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeLeft;
    pthread_mutex_init(&fakeMutex, NULL);
    pthread_mutex_lock(&fakeMutex);
    pthread_cond_timedwait(&chefs[index].chefSignal, &fakeMutex, &ts);
    pthread_mutex_unlock(&fakeMutex);
    // if timedout, then leave (errno == ETIMEDOUT) ig
    if (errno == ETIMEDOUT) // is errno a local/global variable???????????????????????????????
    {
        printf("CHEF %d IS DONE FOR THE DAY\n", index);
        pthread_mutex_destroy(&fakeMutex);
        return 0; // NULL
    }
    for (int i = 0; i < chefs[index].numPizzas; i++)
    {
        for (int j = 0; j < pizzas[chefs[index].pizzaIDs[i]].numIngredients; j++)
        {
            if (pizzas[chefs[index].pizzaIDs[i]].ingredients[j] < 1)
            {
                printf("INGREDIENTS NOT ENUFF for customer %d\n", chefs[index].assignedCustomer);
                goto end;
            }
            ingredients[pizzas[customers[index].pizzaIDs[i]].ingredients[j]]--;
        }
        sleep(3);
        sem_wait(&ovens);
        sleep(pizzas[chefs[index].pizzaIDs[i]].prepTime - 3);
        sem_post(&ovens);
    }
    printf("%d's order has been made by chef %d, now exiting\n", chefs[index].assignedCustomer, index);
end:
    timeUsed = time(0) - timeUsed;
    timeLeft -= timeUsed;
    pthread_mutex_unlock(&chefs[index].busyCheckLock);
    goto repeat;
}

int main()
{
    scanf("%d %d %d %d %d %d", &n, &m, &num, &c, &o, &s);

    chefs = (Chef *)malloc(n * sizeof(Chef));
    pizzas = (Pizza *)malloc((m + 1) * sizeof(Pizza));
    customers = (Customer *)malloc(c * sizeof(Customer));
    ingredients = (int *)malloc((num + 1) * sizeof(int));

    pthread_t customerThreads[c];
    pthread_t chefThreads[n];

    for (int i = 0; i < m; i++)
    {
        int id, time, ingredNum;
        scanf("%d %d %d", &id, &time, &ingredNum);
        pizzas[id].numIngredients = ingredNum;
        pizzas[id].prepTime = time;
        pizzas[id].ingredients = (int *)malloc(pizzas[id].numIngredients * sizeof(int));
        for (int j = 0; j < pizzas[id].numIngredients; j++)
            scanf("%d", &pizzas[id].ingredients[j]);
    }
    for (int i = 0; i < num; i++)
        scanf("%d", &ingredients[i + 1]); // + 1 is important, we are doing 1 indexing for the ingredients
    for (int i = 0; i < n; i++)
    {
        scanf("%d %d", &chefs[i].entryTime, &chefs[i].exitTime);
        pthread_cond_init(&chefs[i].chefSignal, NULL);
        pthread_mutex_init(&chefs[i].busyCheckLock, NULL);
    }
    for (int i = 0; i < c; i++)
    {
        scanf("%d %d", &customers[i].entryTime, &customers[i].numPizzas);
        customers[i].pizzaIDs = (int *)malloc(customers[i].numPizzas * sizeof(int));
        for (int j = 0; j < customers[i].numPizzas; j++)
            scanf("%d", &customers[i].pizzaIDs[j]);
    }

    sem_init(&ovens, 0, o);
    pthread_mutex_init(&numCustomersLock, NULL);
    for (int i = 0; i < c; i++)
    {
        int *a = malloc(sizeof(int));
        *a = i;
        pthread_create(customerThreads[i], NULL, &customerRoutine, a);
    }
    for (int i = 0; i < n; i++)
    {
        int *a = malloc(sizeof(int));
        *a = i;
        pthread_create(&chefThreads[i], NULL, &chefRoutine, a);
    }
    for (int i = 0; i < c; i++)
        pthread_join(&customerThreads[i], NULL);
    for (int i = 0; i < n; i++)
        pthread_join(&chefThreads[i], NULL);

    // destroying everything
    for (int i = 0; i < n; i++)
    {
        pthread_cond_destroy(&chefs[i].chefSignal);
        pthread_mutex_destroy(&chefs[i].busyCheckLock);
    }
    sem_destroy(&ovens);
    pthread_mutex_destroy(&numCustomersLock);

    return 0;
}