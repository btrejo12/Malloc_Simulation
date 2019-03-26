/**********************************************************************************
cs3732p1.c by Brenda Trejo (zpl314)
Purpose: The purpose of this file is to assist functionality of cs3723p1Driver.c in order
    to maintain a storage heap.
Command Parameters: The different functions in this file have various parameters. 
Input: The input to the functions in this file are various structuers defined in 
    the include file "cs3723p1.h" and variables created in cs3723p1Driver.c
Result: The file help the cs3723p1Driver.c manage a block of memeory by allocated and
    freeing various nodes in the heap. 
Returns: The various functions return pointers to memory allocated and size of freed nodes. 
Notes: All added function prototypes were declared in this file. 
**********************************************************************************/
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
/**************************************************************************************
cs3723p1.h
Purpose: The purpose of the include file is to provide all the structures used in both
    cs3723p1.c and cs3723p1Driver.
**************************************************************************************/
#include "cs3723p1.h"

/* Added functions not included in cs3723p1.h */
void addEndSizeFree(FreeNode *pCurrent);
void addEndSizeAlloc(AllocNode *pCurrent);
void removeFromFreeList(FreeNode *pCurrent, StorageManager *pMgr);
void insertIntoFreeList(FreeNode *pNewFreeNode, StorageManager *pMgr, short shItemSize);

/*******************************smAlloc********************************************************
void * smAlloc(StorageManager *pMgr, short shDataSize, char sbData[], SMResult *psmResult)
Purpose: 
    Allocates a block of memory with at least the given size requested and returns a pointer to 
    the data.
Parameters:
    I/O     StorageManager *pMgr: The free list is modified whenever a node is allocated.
    I       short shDataSize: The size of the data to be copied into the node. 
    I       char sbData[]: The actual data to be copied into the node. 
    I/O     SMResult *psmResult: smAlloc uses part of this structure to determine success or failure
                                in allocating a node. 
Return Value: smAlloc returns a charcter pointer to the data in the allocated node. 
*********************************************************************************************/
void * smAlloc(StorageManager *pMgr, short shDataSize, char sbData[], SMResult *psmResult){

    short shAllocSize;                              //The size of the node to be allocated
    short shSizeDifference;                         //The size of the leftover memeory in the free node
    FreeNode *pPrev, *pNext, *pCurrent;             //Pointers to different free nodes
    AllocNode *pAllocNode;                          //The pointer to the node to be allocated
    
    /* Start the pointer at the beginning of the free list */
    pCurrent = pMgr->pFreeList;

    /* Add the overhead size to the requested size */
    shAllocSize = shDataSize + OVERHEAD_SIZE;

    /* Make sure the allocated size is at least the minimum size */
    if (shAllocSize < pMgr->shMinimumSize) {
        shAllocSize = pMgr->shMinimumSize;
    }

    /* Traverse through the free list to find a big enough node. */
    while (pCurrent != NULL) {

        /* If the node is big enough, break to keep the position */
        if ( pCurrent->shItemSize >= shAllocSize ) {
            break;
        }

        /*Move to the next free node in the list */
        pCurrent = pCurrent->pNext;
    }
        /* If no node in the list was big enough, pCurrent will be NULL */
        if (pCurrent == NULL) {
            /*Return faukter */
            psmResult->rc = 901;            //error 901 is not enough memory error
            strcpy(psmResult->szErrorMessage, "Not enough free memory to allocate!!");
            return NULL;
        }
      
        /* Otherwise, there is a node big enough to allocated. Remove it from the free list */
        removeFromFreeList(pCurrent, pMgr);

        /* See what's left of the free node after we allocate it */
        shSizeDifference = pCurrent->shItemSize - shAllocSize;

        /* If the size difference is below the minimum size, then the size
        *  of the allocated node takes up the entire free node */
        if (shSizeDifference < pMgr->shMinimumSize) {
            shAllocSize = pCurrent->shItemSize;
        } else {
            /* The remainder of the free node is big enough for a new free node */
            FreeNode *pNewFreeNode = (FreeNode*)((char*)pCurrent + shAllocSize);
            insertIntoFreeList(pNewFreeNode, pMgr, shSizeDifference);
        }

        /* Set up the new allocated node */
        pAllocNode = (AllocNode*)pCurrent;
        pAllocNode->shItemSize = shAllocSize;
        pAllocNode->cAF = 'A';
        strcpy(pAllocNode->sbAllocDumb, sbData);
        addEndSizeAlloc(pAllocNode);

        /* Return successful */
        psmResult->rc = 0;
        return &(pAllocNode->sbAllocDumb);
}

/********************************************smFree*******************************************
short smFree(StorageManager *pMgr, char *pUserData, SMRsult *psmResult)
Purpose: The purpose of smFree is to free up the specified node that is passed in. 
Parameters:
    I/O     StorageManager *pMgr: The free list is modified whenever a new free node is added.
    I       char *pUserData: The pointer to the data in the node that is scheduled to be freed.
    I/O     SMResult *psmResult: smFree modifies parts of this structure depending on whether
                                freeding the node was a success or failure. 
Return value: smFree returns a short which is the size of the final free node. 
***********************************************************************************************/
short smFree(StorageManager *pMgr, char *pUserData, SMResult *psmResult) {
	

        FreeNode *pFreeNode, *pCombinedFreeNode;            //Temporary free node and final free node
        AllocNode *pAllocNode = (AllocNode*)((char*)pUserData - (sizeof(char) + sizeof(short))); //Alloc node to be freed
        short *pshEndSize;                                  //A pointer to where the end size will be
        short shItemSize = pAllocNode->shItemSize;          //The item size of the new free node
        
        /* Make sure this is an allocated node to be freed */
        if (pAllocNode->cAF != 'A') {
            /* Return failure */
            psmResult->rc = 902;
            strcpy(psmResult->szErrorMessage, "Error: Attempted to free a node which isn't allocated!!\n");
            return 0; 
        }

        /*Set the address of the final free node */
        pCombinedFreeNode = (FreeNode*)pAllocNode; 
       
       /* Move the temporary free node pointer to the right */
       pFreeNode = (FreeNode*)(((char*)pAllocNode) + pAllocNode->shItemSize);
       
       
       /* CHeck to see if the right node is free */
        if (pFreeNode->cAF == 'F') { 
            /* The final node will combine the right node and currrent node. 
            *  Update the size of the final free node */
            shItemSize = shItemSize + pFreeNode->shItemSize;

            /* Remove the right node from the free list */
            removeFromFreeList(pFreeNode, pMgr);
        }

        /* Move the temporary pointer to the left */
        short shEndSize = *(((char*)pAllocNode) - sizeof(short));
        pFreeNode = (FreeNode*)(((char*)pAllocNode) - shEndSize);
        
        /* Check to see if the left node is free */
        if(pFreeNode->cAF == 'F') { 
            /* The final free node will combine the left node and the current node. 
            * Update the size of the final free node */
            shItemSize = shItemSize + pFreeNode->shItemSize;

            /* Remove the left node from the free list */
            removeFromFreeList(pFreeNode, pMgr);

           /* Shift the pointer of the final node left */
            pCombinedFreeNode = pFreeNode;
        }
      
       /* Zero out the node, insert it into the free list, and return it's size */
        memset(pCombinedFreeNode,'\0',shItemSize);
        insertIntoFreeList(pCombinedFreeNode,  pMgr, shItemSize);
        return pCombinedFreeNode->shItemSize;
}

/*******************************smInit***************************************************
void smInit(StorageManager *pMgr)
Purpose: The purpose of smInit is to initialize the current heap as a large free node in
        order to start the storage managing process.
Parameters:
    I/O     StorageManager *pMgr: The free list is modified at the beginning of the program
                                in order to have it point to the heap.

Notes: Everything in pMgr must have been previously initialized in cs3723p1Driver.c
****************************************************************************************/
void smInit(StorageManager *pMgr) {
        /* Clear out the heap with 0 values */
        memset(pMgr->pBeginStorage, '\0',pMgr->shHeapSize);

        /*Set up the heap as one large free node.*/
	FreeNode *pInitialNode =(FreeNode*)((char*) pMgr->pBeginStorage);
	pInitialNode->shItemSize = pMgr->shHeapSize;
	pInitialNode->cAF = 'F';
        
        /*Set StorageManager's pFreelist to point to the initial node. */
        pMgr->pFreeList = pInitialNode;

        /*Add the size of the node to the end */
        addEndSizeFree(pInitialNode);
}

/***************************************addEndSizeAlloc********************************************************
void addEndSizeAlloc(AllocNode *pCurrent)
Purpose: The purpose of addEndSizeAlloc is to add the size of the given node at the end of the node. 
Parameters:
    I/O     AllocNode *pCurrent: The node to which the size will be added at the end. 
Notes: No elements inside the structure are being changed, only the block of memeory where the structure resides. 
        The overhead size accounted for the size to be inputted at the end of the node. 
*************************************************************************************************************/
void addEndSizeAlloc(AllocNode *pCurrent ) {
    /*Address of where the size should be inserted */
    short *pshPrecedingSize = (short*)((((char*)pCurrent) + pCurrent->shItemSize) - sizeof(pCurrent->shItemSize));
    
    /*Insert the size value at the address */
    *pshPrecedingSize = pCurrent->shItemSize;
}

/********************************************addEndSizeFree****************************************************
void addEndSizeFree(FreeNode *pCurrent)
Purpose: The purpose of addEndSizeFree is to add the size of the given node at the end of the node. 
Parameters:
    I/O     FreeNode *pCurrent: The node to which the size will be added at the end. 
Notes: No elements inside the structure are being changed, only the block of memory where the structure resides. 
            The overhead size accounted for the size to be inputted at the end of the node.
**************************************************************************************************************/
void addEndSizeFree(FreeNode *pCurrent) {
    /*Address of where the size should be inserted */
    short *pshPrecedingSize = (short*)((((char*)pCurrent) + pCurrent->shItemSize) - sizeof(pCurrent->shItemSize));

    /*Insert the size value at the address */
    *pshPrecedingSize = pCurrent->shItemSize;
}
/******************************removeFromFreeList********************************************************
void removeFromFreeList(FreeNode *pCurrent, StorageManager *pMgr)
Purpose: The purpose of this function is to remove the specified node from the free list. 
Parameters:
    I       FreeNode *pCurrent: This is the node to be removed from the free list. 
    I/O     StorageManager *pMgr: The free list is used in the case that a new head node was initialized. 
*******************************************************************************************************/
void removeFromFreeList(FreeNode *pCurrent, StorageManager *pMgr) {

        FreeNode *pPrev, *pNext;            //Initialize possible previous and next free node

        /* If the current free node is in the middle of the free list */
        if ( pCurrent->pPrev != NULL && pCurrent->pNext != NULL) { 
            /*Set your other nodes*/
            pPrev = pCurrent->pPrev;
            pNext = pCurrent->pNext;
           
            /* The other nodes must pointer around the current node */
            pPrev->pNext = pNext;
            pNext->pPrev = pPrev;

            /* Zero out current node's pointer for extra precaution */
            pCurrent->pPrev = NULL; 
            pCurrent->pNext = NULL; 

            //The free list doesn't get modified because it's head is elsewhere
        } 
        /* If the current free node is at the end of the free list */
        else if (pCurrent->pPrev != NULL && pCurrent->pNext == NULL) { 
            /* Set applicable nodes */
            pPrev = pCurrent->pPrev;

            /*Change the other node's pointers*/
            pPrev->pNext = NULL; 

            /* Zero out current node's pointer for extra precaution */
            pCurrent->pPrev = NULL; 

            //The free list doesn't get modified bceause's its head is elsewhere
        } 
        /* If the current free node is at the beginning of the free list */
        else if (pCurrent->pPrev == NULL && pCurrent->pNext != NULL){
            /* Set applicable nodes */
            pNext = pCurrent->pNext;
            
            /*Zero out current node's pointers for extra precaution */
            pCurrent->pNext = NULL; 

            /*Change the other node's pointers */
            pNext->pPrev = NULL;

            //The free list has a new head, change that. 
            pMgr->pFreeList = pNext;
        } 
        /* The current node is the only node in the free list */
        else { 
            pMgr->pFreeList = NULL;           
        }
}

/********************************insertIntoFreeList************************************
void insertIntoFreeList(FreeNode *pFreeNode, StorageManager *pMgr, short shItemSize)
Purpose: The purpose of insertIntoFreeList is to insert the specified Free Node into the free list. 
    The function also declares the rest of the elements in the FreeNode structure within this function.
Parameters:
    I       FreeNode *pFreeNode: The free node to be inserted to the front of the free list. 
    I/O     StorageManager *pMgr: The free list is modified whenever a new node is added to the front. 
    I       short shItemSize: The node's size is used when declaring the new node. 
Notes: FreeNode *pCurrent must already have it's initial address initialized and the fucntion will declare the rest
    as well as add the size to the end of it. 
**************************************************************************************/
void insertIntoFreeList(FreeNode *pFreeNode, StorageManager *pMgr, short shItemSize) {
    /*There currently isn't a node in the free list */
    if (pMgr->pFreeList == NULL) {  

        /*Point the free list to this node */
        pMgr->pFreeList = pFreeNode;

        /* Set up the node */
        pFreeNode->cAF = 'F';
        pFreeNode->shItemSize = shItemSize;
        addEndSizeFree(pFreeNode);
    } 
    /* The exists another node that is the head of the free list */
    else {    
        FreeNode *tmpNode = pMgr->pFreeList; //current node at the head of free list
        tmpNode->pPrev = pFreeNode; //declare the current node to be it's previous
        pMgr->pFreeList = pFreeNode; //Head of the list is now the current node
        pFreeNode->pNext = tmpNode; //new node's next is previously free list leader

        /* Set up the node */
        pFreeNode->shItemSize = shItemSize;
        pFreeNode->cAF = 'F';
        addEndSizeFree(pFreeNode);
    }
}
