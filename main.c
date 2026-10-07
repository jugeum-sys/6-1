#define _CRT_SECURE_NO_WARNINGS   /* Visual Studio C4996 방지 */

/*
 * avl_search.c
 * ---------------------------------------------------------------
 * 과제 06 - 순차 탐색, 이진 탐색 트리와 AVL 트리의 성능 비교
 *
 *  1. 0~1000 사이의 정수를 100번 생성하여 같은 순서로
 *     배열, 이진 탐색 트리(BST), AVL 트리에 각각 저장을 시도한다.
 *     이미 저장된 값과 중복되면 저장하지 않는다.
 *  2. 생성(삽입) 과정에서 발생한 숫자 비교 횟수를 자료구조별로 측정한다.
 *  3. 삽입이 끝난 뒤 배열의 길이, BST의 높이, AVL 트리의 높이를 출력한다.
 *  4. 0~1000 사이의 정수 50개를 탐색 대상으로 생성해 세 가지 탐색을 모두 수행하고
 *     각각의 비교 횟수와 총합·평균을 출력한다.
 *
 * [숫자 비교를 세는 기준]
 *   새로 생성된 값(또는 탐색 대상)을 이미 저장된 값 하나와 맞대어 보는 것을 1회로 센다.
 *   배열은 원소 하나, 트리는 노드 하나를 거칠 때마다 1회다.
 *   중복이어서 삽입되지 않은 경우에도 중복임을 확인하는 데 쓴 비교는 포함한다.
 *   AVL 트리의 높이 계산, balance factor 확인, 회전 판단은 값끼리의 비교가 아니므로
 *   세지 않는다. (회전 여부도 값이 아니라 높이만으로 판단하도록 구현했다)
 *
 * [트리의 높이]
 *   과제 정의대로 노드 수 기준이다. 빈 트리는 0, 루트만 있으면 1이다.
 *
 * [난수 생성]
 *   time.h와 rand()를 쓰지 않고 선형 합동법(LCG)으로 직접 만들었다.
 *   시작할 때 시드를 입력하면 다른 데이터로, Enter만 누르면 기본 시드로 재현된다.
 */

#include <stdio.h>
#include <stdlib.h>

#define TRIAL_COUNT   100     /* 난수 생성(삽입 시도) 횟수 */
#define SEARCH_COUNT   50     /* 탐색 대상 개수 */
#define MAX_VALUE    1000     /* 생성하는 정수의 최댓값 */
#define DEFAULT_SEED 12345    /* 기본 시드 */

 /* ---------------- 난수 생성기 (선형 합동법) ---------------- */

static unsigned long rngState = DEFAULT_SEED;

static void rng_seed(unsigned long seed) { rngState = seed; }

static int rng_next(int bound) {
    rngState = rngState * 1103515245UL + 12345UL;
    return (int)((rngState >> 16) % (unsigned long)bound);
}

/* ---------------- 이진 탐색 트리 ---------------- */

typedef struct BSTNode {
    int             data;
    struct BSTNode* left;
    struct BSTNode* right;
} BSTNode;

static BSTNode* bstNodes[TRIAL_COUNT];
static int      bstNodeCount = 0;

static BSTNode* bst_new_node(int data) {
    BSTNode* node = (BSTNode*)malloc(sizeof(BSTNode));
    if (node == NULL) return NULL;
    node->data = data;
    node->left = node->right = NULL;
    bstNodes[bstNodeCount++] = node;
    return node;
}

/*
 * BST 삽입. 중복이면 삽입하지 않는다.
 * 노드 하나와 비교할 때마다 *comparisons 를 증가시킨다.
 */
static BSTNode* bst_insert(BSTNode* root, int value, long* comparisons) {
    BSTNode* cur = root, * parent = NULL;
    int goLeft = 0;

    if (root == NULL) return bst_new_node(value);   /* 첫 값은 비교 없이 루트 */

    while (cur != NULL) {
        (*comparisons)++;                           /* 노드 하나와 비교 */
        parent = cur;
        if (value < cur->data) { cur = cur->left;  goLeft = 1; }
        else if (value > cur->data) { cur = cur->right; goLeft = 0; }
        else                        return root;    /* 중복이므로 삽입하지 않음 */
    }
    if (goLeft) parent->left = bst_new_node(value);
    else        parent->right = bst_new_node(value);
    return root;
}

static int bst_search(BSTNode* root, int key, int* comparisons) {
    BSTNode* cur = root;
    *comparisons = 0;
    while (cur != NULL) {
        (*comparisons)++;
        if (key == cur->data)     return 1;
        else if (key < cur->data) cur = cur->left;
        else                      cur = cur->right;
    }
    return 0;
}

/* 높이(노드 수 기준)와 노드 개수를 반복문으로 계산 */
static int bst_height(BSTNode* root) {
    BSTNode* stack[TRIAL_COUNT + 1];
    int levels[TRIAL_COUNT + 1];
    int top = -1, maxLevel = 0;

    if (root != NULL) { stack[++top] = root; levels[top] = 1; }
    while (top >= 0) {
        BSTNode* node = stack[top];
        int level = levels[top--];
        if (level > maxLevel) maxLevel = level;
        if (node->left != NULL) { stack[++top] = node->left;  levels[top] = level + 1; }
        if (node->right != NULL) { stack[++top] = node->right; levels[top] = level + 1; }
    }
    return maxLevel;
}

/* ---------------- AVL 트리 ---------------- */

typedef struct AVLNode {
    int             data;
    struct AVLNode* left;
    struct AVLNode* right;
    int             height;   /* 노드 수 기준 높이 (단말 노드 = 1) */
} AVLNode;

static AVLNode* avlNodes[TRIAL_COUNT];
static int      avlNodeCount = 0;

static AVLNode* avl_new_node(int data) {
    AVLNode* node = (AVLNode*)malloc(sizeof(AVLNode));
    if (node == NULL) return NULL;
    node->data = data;
    node->left = node->right = NULL;
    node->height = 1;
    avlNodes[avlNodeCount++] = node;
    return node;
}

static int avl_height(AVLNode* node) { return (node == NULL) ? 0 : node->height; }

static void avl_update_height(AVLNode* node) {
    int hl = avl_height(node->left), hr = avl_height(node->right);
    node->height = ((hl > hr) ? hl : hr) + 1;
}

/* balance factor = 왼쪽 높이 - 오른쪽 높이 (값끼리의 비교가 아니므로 세지 않음) */
static int avl_balance(AVLNode* node) {
    return (node == NULL) ? 0 : avl_height(node->left) - avl_height(node->right);
}

static AVLNode* rotate_right(AVLNode* y) {       /* LL 회전 */
    AVLNode* x = y->left;
    y->left = x->right;
    x->right = y;
    avl_update_height(y);
    avl_update_height(x);
    return x;
}

static AVLNode* rotate_left(AVLNode* x) {        /* RR 회전 */
    AVLNode* y = x->right;
    x->right = y->left;
    y->left = x;
    avl_update_height(x);
    avl_update_height(y);
    return y;
}

/*
 * AVL 삽입. 삽입 위치를 찾는 방법은 BST와 같고, 중복이면 삽입하지 않는다.
 * 삽입 후 균형이 깨졌으면 회전으로 복구한다.
 * 회전 종류는 값이 아니라 balance factor(높이)만으로 판단하므로 비교 횟수에 포함되지 않는다.
 */
static AVLNode* avl_insert(AVLNode* node, int value, long* comparisons) {
    int balance;

    if (node == NULL) return avl_new_node(value);

    (*comparisons)++;                                   /* 노드 하나와 비교 */
    if (value < node->data) {
        node->left = avl_insert(node->left, value, comparisons);
    }
    else if (value > node->data) {
        node->right = avl_insert(node->right, value, comparisons);
    }
    else {
        return node;                                    /* 중복이므로 삽입하지 않음 */
    }

    avl_update_height(node);
    balance = avl_balance(node);

    if (balance > 1) {                                  /* 왼쪽이 무거움 */
        if (avl_balance(node->left) < 0)                /* LR : 자식을 먼저 왼쪽 회전 */
            node->left = rotate_left(node->left);
        return rotate_right(node);                      /* LL */
    }
    if (balance < -1) {                                 /* 오른쪽이 무거움 */
        if (avl_balance(node->right) > 0)               /* RL : 자식을 먼저 오른쪽 회전 */
            node->right = rotate_right(node->right);
        return rotate_left(node);                       /* RR */
    }
    return node;
}

/* AVL 탐색: 방법 자체는 BST 탐색과 같다 */
static int avl_search(AVLNode* root, int key, int* comparisons) {
    AVLNode* cur = root;
    *comparisons = 0;
    while (cur != NULL) {
        (*comparisons)++;
        if (key == cur->data)     return 1;
        else if (key < cur->data) cur = cur->left;
        else                      cur = cur->right;
    }
    return 0;
}

/* ---------------- 배열 ---------------- */

/*
 * 배열에서 중복을 확인하는 순차 탐색.
 * 찾으면 1, 없으면 0을 반환하고 비교 횟수를 *comparisons 에 넣는다.
 */
static int array_search(const int array[], int size, int key, int* comparisons) {
    int i;
    *comparisons = 0;
    for (i = 0; i < size; i++) {
        (*comparisons)++;                               /* 원소 하나와 비교 */
        if (array[i] == key) return 1;
    }
    return 0;
}

/* ---------------- 시드 입력 ---------------- */

static unsigned long ask_seed(void) {
    char line[64];
    unsigned long value = 0;
    int i = 0, digits = 0;

    printf("난수 시드를 입력하세요 (그냥 Enter를 누르면 %d): ", DEFAULT_SEED);
    if (fgets(line, sizeof(line), stdin) == NULL) return DEFAULT_SEED;

    while (line[i] == ' ' || line[i] == '\t') i++;
    while (line[i] >= '0' && line[i] <= '9') {
        value = value * 10 + (unsigned long)(line[i] - '0');
        digits++;
        i++;
    }
    return (digits > 0) ? value : DEFAULT_SEED;
}

/* ---------------- main ---------------- */

int main(void) {
    int generated[TRIAL_COUNT];       /* 생성된 100개의 정수 (발생 순서) */
    int array[TRIAL_COUNT];           /* 중복 없이 저장되는 배열 */
    int arrayLength = 0;
    int keys[SEARCH_COUNT];

    BSTNode* bstRoot = NULL;
    AVLNode* avlRoot = NULL;

    long arrayBuild = 0, bstBuild = 0, avlBuild = 0;
    long seqTotal = 0, bstTotal = 0, avlTotal = 0;
    long seqFoundTotal = 0, bstFoundTotal = 0, avlFoundTotal = 0;
    long seqFailTotal = 0, bstFailTotal = 0, avlFailTotal = 0;
    int  duplicates = 0, foundCount = 0, mismatch = 0;
    int  i, value, comp, seqComp, bstComp, avlComp;
    int  seqFound, bstFound, avlFound;

    rng_seed(ask_seed());

    /* ---------- 1. 난수 100개 생성 및 세 자료구조에 저장 ---------- */
    for (i = 0; i < TRIAL_COUNT; i++) {
        value = rng_next(MAX_VALUE + 1);                /* 0 ~ 1000 */
        generated[i] = value;

        /* [1] 배열 : 처음부터 끝까지 순차 탐색으로 중복 확인 후 뒤에 추가 */
        if (array_search(array, arrayLength, value, &comp)) {
            duplicates++;                               /* 이미 있으면 저장하지 않음 */
        }
        else {
            array[arrayLength++] = value;
        }
        arrayBuild += comp;

        /* [2] BST, [3] AVL : 같은 값을 같은 순서로 삽입 */
        bstRoot = bst_insert(bstRoot, value, &bstBuild);
        avlRoot = avl_insert(avlRoot, value, &avlBuild);
    }

    printf("\n[1] 생성된 %d개의 정수 (발생 순서)\n", TRIAL_COUNT);
    for (i = 0; i < TRIAL_COUNT; i++) {
        printf("%5d", generated[i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }

    /* ---------- 2. 저장 결과와 생성 비용 ---------- */
    printf("\n[2] 저장 결과\n");
    printf("  Stored values        : %d\n", arrayLength);
    printf("  Duplicated (skipped) : %d\n\n", duplicates);

    printf("  Construction\n");
    printf("    Array comparisons : %6ld\n", arrayBuild);
    printf("    BST comparisons   : %6ld\n", bstBuild);
    printf("    AVL comparisons   : %6ld\n\n", avlBuild);

    printf("  Structure\n");
    printf("    Array length : %3d\n", arrayLength);
    printf("    BST height   : %3d\n", bst_height(bstRoot));
    printf("    AVL height   : %3d\n", avl_height(avlRoot));
    printf("  (세 자료구조에 저장된 값의 수: 배열 %d, BST %d, AVL %d)\n",
        arrayLength, bstNodeCount, avlNodeCount);

    /* ---------- 3. 탐색 대상 50개 생성 ---------- */
    for (i = 0; i < SEARCH_COUNT; i++) keys[i] = rng_next(MAX_VALUE + 1);

    printf("\n[3] 생성된 탐색 대상 %d개\n", SEARCH_COUNT);
    for (i = 0; i < SEARCH_COUNT; i++) {
        printf("%5d", keys[i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }

    /* ---------- 4. 탐색 수행 ---------- */
    printf("\n[4] 탐색 결과 (%d회)\n", SEARCH_COUNT);
    printf("  No.    Key   Result        Seq    BST    AVL\n");
    for (i = 0; i < SEARCH_COUNT; i++) {
        seqFound = array_search(array, arrayLength, keys[i], &seqComp);
        bstFound = bst_search(bstRoot, keys[i], &bstComp);
        avlFound = avl_search(avlRoot, keys[i], &avlComp);
        if (seqFound != bstFound || seqFound != avlFound) mismatch++;

        printf("  %3d  %5d   %-9s  %5d  %5d  %5d\n",
            i + 1, keys[i], seqFound ? "Found" : "Not Found",
            seqComp, bstComp, avlComp);

        seqTotal += seqComp;  bstTotal += bstComp;  avlTotal += avlComp;
        if (seqFound) {
            foundCount++;
            seqFoundTotal += seqComp; bstFoundTotal += bstComp; avlFoundTotal += avlComp;
        }
        else {
            seqFailTotal += seqComp;  bstFailTotal += bstComp;  avlFailTotal += avlComp;
        }
    }

    /* ---------- 5. 요약 ---------- */
    printf("\n[5] 요약\n");
    printf("  Searches : %d\n\n", SEARCH_COUNT);
    printf("  Sequential Search\n");
    printf("    Total comparisons   : %ld\n", seqTotal);
    printf("    Average comparisons : %.2f\n\n", (double)seqTotal / SEARCH_COUNT);
    printf("  BST Search\n");
    printf("    Total comparisons   : %ld\n", bstTotal);
    printf("    Average comparisons : %.2f\n\n", (double)bstTotal / SEARCH_COUNT);
    printf("  AVL Search\n");
    printf("    Total comparisons   : %ld\n", avlTotal);
    printf("    Average comparisons : %.2f\n", (double)avlTotal / SEARCH_COUNT);

    /* ---------- 6. 추가 통계 ---------- */
    printf("\n[6] 추가 통계\n");
    printf("  탐색 성공 / 실패 : %d / %d\n", foundCount, SEARCH_COUNT - foundCount);
    if (foundCount > 0) {
        printf("  성공한 탐색의 평균 비교 : 순차 %.2f, BST %.2f, AVL %.2f\n",
            (double)seqFoundTotal / foundCount,
            (double)bstFoundTotal / foundCount,
            (double)avlFoundTotal / foundCount);
    }
    if (SEARCH_COUNT - foundCount > 0) {
        printf("  실패한 탐색의 평균 비교 : 순차 %.2f, BST %.2f, AVL %.2f\n",
            (double)seqFailTotal / (SEARCH_COUNT - foundCount),
            (double)bstFailTotal / (SEARCH_COUNT - foundCount),
            (double)avlFailTotal / (SEARCH_COUNT - foundCount));
    }
    printf("  생성 + 탐색 총 비교 : 배열 %ld, BST %ld, AVL %ld\n",
        arrayBuild + seqTotal, bstBuild + bstTotal, avlBuild + avlTotal);
    printf("  세 탐색의 성공/실패 결과 일치 : %s\n", (mismatch == 0) ? "예" : "아니오");

    for (i = 0; i < bstNodeCount; i++) free(bstNodes[i]);
    for (i = 0; i < avlNodeCount; i++) free(avlNodes[i]);
    return 0;
}