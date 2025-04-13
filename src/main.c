#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

/* Intializations ===================================================================================================== */
#define ITERATIONS 50           /* How many iterations to do */
#define HASH_SIZE 503           /* The size of the hash table (the bigger the faster the program will execute) */
#define BASE_RANK 0.15          /* The base rank of each page rank */
#define DAMPING_FACTOR 0.85     /* The damping factor of each page rank */

struct Node;
struct listNode;

typedef struct Node Node;
typedef struct listNode listNode;

/* The struct of a node */
struct Node {
  long int NodeID;        /* The node ID */
  double PageRank;        /* The page rank of the node */
  unsigned int OutGoing;  /* How many outgoing nodes the current node has */
  listNode *Connections;  /* The list of the other nodes the current node is pointing to */
};

/* A list of a Node, for memmory management purposes */
struct listNode {
  Node *Node;
  listNode *Next;
};

Node *createNode(long int);
void addConnection(Node*, Node*);
Node *locateNode(long int);
void ReadFile(FILE*);
void* pageRank(void*);

listNode **Nodes;             /* hash table containing all of the nodes we have yet initialized */
int NUM_OF_THREADS;           /* The ammount of threads to use */
pthread_t *threads;           /* array with the actual threads */
pthread_cond_t DoneCV;        /* Conditional variable if we are done with the page rank calculation */
pthread_mutex_t DoneM;        /* Mutex on if we are done with the page rank calculation */
pthread_barrier_t barrier;    /* Barrier to make sure all the threads are finished before continuing to another iteration */
atomic_int hashPos;           /* the next free possition within the hash table */
atomic_int isDone;            /* If we are finished with the page rank calculation */
atomic_int threadsDone;       /* How many threads have finished with the current iteration */
int maxNodeID = -1;           /* The biggest yet given Node ID */

/* Functions ========================================================================================================== */
/*  
 * Creates and returns a pointer to a new node with its ID set as given
 * also adds it to the hash table
 */ 
Node *createNode(long int NodeID) {
  int tablePos;
  Node *newNode;
  listNode *newListNode;

  /* Creating the node */
  newNode = (Node*) malloc(sizeof(Node));
  if (newNode == NULL) {
    printf("Error while allocating memory\n");
    exit(1);
  }
  newNode->NodeID = NodeID;
  newNode->PageRank = 1.0;
  newNode->OutGoing = 0;
  newNode->Connections = NULL;

  /* Allocating memory for the list node */
  newListNode = (listNode*) malloc(sizeof(listNode));
  if (newListNode == NULL) {
    printf("Error while allocating memory for list node\n");
    exit(1);
  }

  /* Adding the new node to the hash table */
  tablePos = NodeID % HASH_SIZE;
  newListNode->Node = newNode;
  newListNode->Next = Nodes[tablePos];
  Nodes[tablePos] = newListNode;

  return newNode;
}

/*
 * Adds a connection from the node 'from' to the node 'to'
 */ 
void addConnection(Node *to, Node *from) {
  listNode *newNodeList;
  if (to == NULL || from == NULL) {
    printf("Error: inpout is NULL\n");
    return;
  }

  /* Creating the new node for the list */
  newNodeList = (listNode*) malloc(sizeof(listNode));
  if (newNodeList == NULL) {
    printf("Error while allocating memory\n");
    exit(1);
  }

  /* Adding 'from' node at the beginning of 'to' list */
  newNodeList->Node = from;
  newNodeList->Next = to->Connections;
  to->Connections = newNodeList;
}

/* 
 * Finds the node inside the hash table and returns it
 * if it could not find the node it returns NULL
 */
Node *locateNode(long int nodeID) {
  int tablePos = nodeID % HASH_SIZE;
  listNode *current = Nodes[tablePos];

  /* Traverse the linked list at this position */
  while (current != NULL) {
    if (current->Node->NodeID == nodeID) {
      return current->Node;
    }
    current = current->Next;
  }

  /* If we could not find it */
  return NULL;
}

/* Reads the file and initializes everything */
void ReadFile(FILE *file) {
  char buffer[100];
  long int num1,num2;
  Node *from,*to;

  /* Parsing throught the file */
  while(fgets(buffer,sizeof(buffer),file)) {
    if(buffer[0] == '#' || buffer[0] == '\n') {
      continue;
    }

    if (sscanf(buffer,"%ld %ld",&num1,&num2) != 2) {
      printf("Error while reading from file!\n");
      exit(1);
    }
    
    /* Locating the nodes inside the hash table */
    from = locateNode(num1);
    if (from == NULL)
      from = createNode(num1);
    if (num1 > maxNodeID)
      maxNodeID = num1;
    from->OutGoing++;

    to = locateNode(num2);
    if (to == NULL)
      to = createNode(num2);
    if (num2 > maxNodeID)
      maxNodeID = num2;
    
    /* Creating the connection */
    addConnection(to,from);
  }
}

/* Page Rank ========================================================================================================== */
void* pageRank(void* args) {
  int pos,i;
  double newPG;
  listNode *chain,*nodeConnections;

  for (i = 0; i < ITERATIONS; i++) {
    /* Going throught all the possition in the hash table */
    while ((pos = atomic_fetch_add(&hashPos, 1)) < HASH_SIZE) {
      chain = Nodes[pos];
      /* Going throught the chain in the hash table */
      while (chain != NULL) {
        newPG = 0;
        nodeConnections = chain->Node->Connections;
        
        /* Parsing throught the connections of the current Node */
        while (nodeConnections != NULL) {
          newPG += (nodeConnections->Node->PageRank / nodeConnections->Node->OutGoing);
          nodeConnections = nodeConnections->Next;
        }
        /* updating the page rank of the current node */
        newPG = BASE_RANK + DAMPING_FACTOR * newPG;
        atomic_store(&chain->Node->PageRank, newPG);
        
        /* going to the next node in the chain */
        chain = chain->Next;
      }
    }
    
    /* Syncronizing the threads before resseting */
    pthread_barrier_wait(&barrier);
    
    /* Only allowing one thread to resset the possition */
    if (pthread_self() == threads[0]) { 
      atomic_store(&hashPos, 0);
    }
    
    /* Syncronizing the threads before continuing */
    pthread_barrier_wait(&barrier);
  }
  
  /* Ensure only one thread sets isDone = 1 */
  if (atomic_fetch_add(&threadsDone, 1) == NUM_OF_THREADS - 1) {
    atomic_store(&isDone, 1);
    pthread_cond_signal(&DoneCV);
  }
}




/* Main function ====================================================================================================== */
int main(int argc, char **argv) {
  FILE *in,*out;        /* The input and output file */
  Node* node;           /* a temporary node used for freeing the allocated memmory */
  listNode *chain1,*chain2,*prev1,*prev2;
                        /* some list nodes also use for freeing allocated memmory */
  int i;                /* Integer used for fo loops */
  long int num1,num2;   /* Temporary integers used for writing the output file */
  char buffer[100];     /* Temporary buffer used for writing the output file */
                        /* Variables to count the execution time */
  struct timespec start, end;
  double elapsed,totalTime=0.0;
  
  if (argc != 3) {
    printf("Please input the file name and the number of threads!\n");
    exit(1);
  }

  /* Opening the file */
  in = fopen(argv[1],"r");
  if (in == NULL) {
    printf("Error while opening file! input: %s\n",argv[1]);
    exit(1);
  }

  /* intializing the hash table */
  Nodes = (listNode**) malloc(sizeof(listNode*) * HASH_SIZE);
  if (Nodes == NULL) {
    printf("Error while allocating memmory\n");
    exit(1);
  }

  /* Intializing the threads */
  NUM_OF_THREADS = atoi(argv[2]);
  threads = (pthread_t*) malloc(sizeof(pthread_t) * NUM_OF_THREADS);
  if (threads == NULL) {
    printf("Error while allocating threads");
    exit(1);
  }

  /* ---------- Reading the file ---------- */
  clock_gettime(CLOCK_MONOTONIC, &start);
  
  ReadFile(in);
  
  clock_gettime(CLOCK_MONOTONIC, &end);
  elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  totalTime += elapsed;
  
  printf("\rReading file: \033[32mDONE!, time: %.6fs \033[0m\n", elapsed);
  
  /* ---------- Calculating the page rank ---------- */
  clock_gettime(CLOCK_MONOTONIC, &start);
  atomic_store(&hashPos,0);
  atomic_store(&isDone,0);
  atomic_store(&threadsDone,0);
  
  pthread_barrier_init(&barrier,NULL,NUM_OF_THREADS);
  pthread_cond_init(&DoneCV,NULL);
  pthread_mutex_init(&DoneM,NULL);
  
  /* Intializing the threads */
  for (i = 0; i < NUM_OF_THREADS; i++) {
    pthread_create(&threads[i],NULL,&pageRank,NULL);
  }
  
  /* Waiting until we are done */
  pthread_mutex_lock(&DoneM);
  while (atomic_load(&isDone) == 0) {
    pthread_cond_wait(&DoneCV,&DoneM);
  }
  pthread_mutex_unlock(&DoneM);
  
  /* Rejoining the threads */
  for (i = 0; i < NUM_OF_THREADS; i++) {
    pthread_join(threads[i],NULL);
  }
  
  clock_gettime(CLOCK_MONOTONIC, &end);
  elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  totalTime += elapsed;

  printf("\rCalculating Page Rank: \033[32mDONE!, time: %.6fs \033[0m\n", elapsed);

  /* ---------- Writting it on the new file ---------- */
  clock_gettime(CLOCK_MONOTONIC, &start);
 
  out = fopen("pagerank.csv","w");
  if (out == NULL) {
    printf("Error while creating output file!\n");
    exit(1);
  }

  /* Because we have everything stored inside of the hash table by the end the Nodes are all
   * mixed up, to fix this we read the input file again, and find one after the other the 
   * Nodes so that we can print them back into the output file in the same order as they where
   * in the input file */

  fprintf(out,"node,pagerank\n");

  /* Parsing throught all the nodes to write them in the output file  */
  for (i = 0; i < maxNodeID; i++) {
    node = locateNode(i);
    if (node == NULL)
      continue;
    fprintf(out,"%ld,%f\n",node->NodeID,node->PageRank);
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
  totalTime += elapsed;
  
  printf("\rWritting Data: \033[32mDONE!, time: %.6fs \033[0m\n", elapsed);
  printf("Total time: %.6f\n",totalTime);
  
  /* Freing/closing everything */
  /* parsing thought he hash table and freeing all the nodes */
  for (i = 0;i < HASH_SIZE; i++) {
    chain1 = Nodes[i];
    prev1 = NULL;
    /* parsing thought the nodes inside the hash table */ 
    while (chain1 != NULL) {
      prev1 = chain1;
      chain1 = chain1->Next;
      chain2 = prev1->Node->Connections;
      prev2 = NULL;
      /* Parsing thought the Connections inside the has table */
      while (chain2 != NULL) {
        prev2 = chain2;
        chain2 = chain2->Next;
        free(prev2);
      }
      free(prev1->Node);
      free(prev1);
    }
  }
  free(threads);
  fclose(in);
  fclose(out);
  free(Nodes);
}

