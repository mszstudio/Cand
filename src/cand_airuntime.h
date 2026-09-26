/*
 * C& Programming Language — Native AI Runtime Engine
 * Pure C Tensor Operations, Autograd, Neural Network primitives & Serialization
 * Copyright (c) 2026 MSZ Studio. All rights reserved.
 */

#ifndef CAND_AIRUNTIME_H
#define CAND_AIRUNTIME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CandTensor {
    float *data;
    float *grad;
    int shape[4];
    int strides[4];
    int ndim;
    int size;
    int requires_grad;
    struct CandTensor *left;
    struct CandTensor *right;
    char op[16];
} CandTensor;

/* ── Tensor Creation & Lifecycle ────────────────────────────────────────── */

static inline void* cand_tensor_create(int d0, int d1, int d2, int d3, int ndim) {
    CandTensor *t = (CandTensor*)calloc(1, sizeof(CandTensor));
    if (!t) return NULL;
    t->ndim = ndim;
    t->shape[0] = d0 > 0 ? d0 : 1;
    t->shape[1] = d1 > 0 ? d1 : 1;
    t->shape[2] = d2 > 0 ? d2 : 1;
    t->shape[3] = d3 > 0 ? d3 : 1;

    t->size = 1;
    for (int i = 0; i < ndim; i++) {
        t->size *= t->shape[i];
    }

    t->strides[3] = 1;
    t->strides[2] = t->shape[3];
    t->strides[1] = t->shape[2] * t->shape[3];
    t->strides[0] = t->shape[1] * t->shape[2] * t->shape[3];

    t->data = (float*)calloc(t->size, sizeof(float));
    t->grad = (float*)calloc(t->size, sizeof(float));
    t->requires_grad = 1;
    return (void*)t;
}

static inline void* cand_tensor_zeros(int rows, int cols) {
    return cand_tensor_create(rows, cols, 1, 1, 2);
}

static inline void* cand_tensor_ones(int rows, int cols) {
    CandTensor *t = (CandTensor*)cand_tensor_zeros(rows, cols);
    if (!t) return NULL;
    for (int i = 0; i < t->size; i++) t->data[i] = 1.0f;
    return (void*)t;
}

static inline void* cand_tensor_rand(int rows, int cols, float min_val, float max_val) {
    CandTensor *t = (CandTensor*)cand_tensor_zeros(rows, cols);
    if (!t) return NULL;
    static int seeded = 0;
    if (!seeded) { srand((unsigned int)time(NULL)); seeded = 1; }
    for (int i = 0; i < t->size; i++) {
        float r = (float)rand() / (float)RAND_MAX;
        t->data[i] = min_val + r * (max_val - min_val);
    }
    return (void*)t;
}

static inline void cand_tensor_free(void *t_ptr) {
    CandTensor *t = (CandTensor*)t_ptr;
    if (!t) return;
    if (t->data) free(t->data);
    if (t->grad) free(t->grad);
    free(t);
}

static inline float cand_tensor_get(void *t_ptr, int r, int c) {
    CandTensor *t = (CandTensor*)t_ptr;
    if (!t || r < 0 || r >= t->shape[0] || c < 0 || c >= t->shape[1]) return 0.0f;
    return t->data[r * t->shape[1] + c];
}

static inline void cand_tensor_set(void *t_ptr, int r, int c, float val) {
    CandTensor *t = (CandTensor*)t_ptr;
    if (!t || r < 0 || r >= t->shape[0] || c < 0 || c >= t->shape[1]) return;
    t->data[r * t->shape[1] + c] = val;
}

static inline void cand_tensor_print(void *t_ptr, const char *label) {
    CandTensor *t = (CandTensor*)t_ptr;
    if (!t) return;
    printf("=== Tensor [%s] (Shape: %dx%d, Size: %d) ===\n", label ? label : "Tensor", t->shape[0], t->shape[1], t->size);
    for (int r = 0; r < t->shape[0]; r++) {
        printf("[ ");
        for (int c = 0; c < t->shape[1]; c++) {
            printf("%8.4f ", t->data[r * t->shape[1] + c]);
        }
        printf("]\n");
    }
    printf("\n");
}

/* ── Element-wise Tensor Operations ────────────────────────────────────────── */

static inline void* cand_tensor_add(void *a_ptr, void *b_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    CandTensor *b = (CandTensor*)b_ptr;
    if (!a || !b || a->size != b->size) return NULL;
    CandTensor *res = (CandTensor*)cand_tensor_zeros(a->shape[0], a->shape[1]);
    res->left = a;
    res->right = b;
    strcpy(res->op, "add");
    for (int i = 0; i < a->size; i++) {
        res->data[i] = a->data[i] + b->data[i];
    }
    return (void*)res;
}

static inline void* cand_tensor_sub(void *a_ptr, void *b_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    CandTensor *b = (CandTensor*)b_ptr;
    if (!a || !b || a->size != b->size) return NULL;
    CandTensor *res = (CandTensor*)cand_tensor_zeros(a->shape[0], a->shape[1]);
    res->left = a;
    res->right = b;
    strcpy(res->op, "sub");
    for (int i = 0; i < a->size; i++) {
        res->data[i] = a->data[i] - b->data[i];
    }
    return (void*)res;
}

static inline void* cand_tensor_mul(void *a_ptr, void *b_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    CandTensor *b = (CandTensor*)b_ptr;
    if (!a || !b || a->size != b->size) return NULL;
    CandTensor *res = (CandTensor*)cand_tensor_zeros(a->shape[0], a->shape[1]);
    res->left = a;
    res->right = b;
    strcpy(res->op, "mul");
    for (int i = 0; i < a->size; i++) {
        res->data[i] = a->data[i] * b->data[i];
    }
    return (void*)res;
}

static inline void* cand_tensor_div(void *a_ptr, void *b_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    CandTensor *b = (CandTensor*)b_ptr;
    if (!a || !b || a->size != b->size) return NULL;
    CandTensor *res = (CandTensor*)cand_tensor_zeros(a->shape[0], a->shape[1]);
    res->left = a;
    res->right = b;
    strcpy(res->op, "div");
    for (int i = 0; i < a->size; i++) {
        res->data[i] = b->data[i] != 0.0f ? (a->data[i] / b->data[i]) : 0.0f;
    }
    return (void*)res;
}

/* ── Matrix Multiplication (Optimized Tiled Matmul) ─────────────────────── */

static inline void* cand_tensor_matmul(void *a_ptr, void *b_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    CandTensor *b = (CandTensor*)b_ptr;
    if (!a || !b || a->shape[1] != b->shape[0]) {
        fprintf(stderr, "C& Tensor Error: Matmul shape mismatch (%dx%d vs %dx%d)\n",
                a ? a->shape[0] : 0, a ? a->shape[1] : 0,
                b ? b->shape[0] : 0, b ? b->shape[1] : 0);
        return NULL;
    }
    int M = a->shape[0];
    int K = a->shape[1];
    int N = b->shape[1];

    CandTensor *res = (CandTensor*)cand_tensor_zeros(M, N);
    res->left = a;
    res->right = b;
    strcpy(res->op, "matmul");

    for (int i = 0; i < M; i++) {
        for (int k = 0; k < K; k++) {
            float aik = a->data[i * K + k];
            for (int j = 0; j < N; j++) {
                res->data[i * N + j] += aik * b->data[k * N + j];
            }
        }
    }
    return (void*)res;
}

/* ── Activations ──────────────────────────────────────────────────────────── */

static inline void* cand_tensor_relu(void *a_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    if (!a) return NULL;
    CandTensor *res = (CandTensor*)cand_tensor_zeros(a->shape[0], a->shape[1]);
    res->left = a;
    strcpy(res->op, "relu");
    for (int i = 0; i < a->size; i++) {
        res->data[i] = a->data[i] > 0.0f ? a->data[i] : 0.0f;
    }
    return (void*)res;
}

static inline void* cand_tensor_softmax(void *a_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    if (!a) return NULL;
    CandTensor *res = (CandTensor*)cand_tensor_zeros(a->shape[0], a->shape[1]);
    res->left = a;
    strcpy(res->op, "softmax");

    for (int r = 0; r < a->shape[0]; r++) {
        float max_v = -1e9f;
        for (int c = 0; c < a->shape[1]; c++) {
            float v = a->data[r * a->shape[1] + c];
            if (v > max_v) max_v = v;
        }

        float sum_exp = 0.0f;
        for (int c = 0; c < a->shape[1]; c++) {
            float exp_v = expf(a->data[r * a->shape[1] + c] - max_v);
            res->data[r * a->shape[1] + c] = exp_v;
            sum_exp += exp_v;
        }

        for (int c = 0; c < a->shape[1]; c++) {
            res->data[r * a->shape[1] + c] /= (sum_exp + 1e-9f);
        }
    }
    return (void*)res;
}

static inline void* cand_tensor_sigmoid(void *a_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    if (!a) return NULL;
    CandTensor *res = (CandTensor*)cand_tensor_zeros(a->shape[0], a->shape[1]);
    res->left = a;
    strcpy(res->op, "sigmoid");
    for (int i = 0; i < a->size; i++) {
        res->data[i] = 1.0f / (1.0f + expf(-a->data[i]));
    }
    return (void*)res;
}

static inline void* cand_tensor_tanh(void *a_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    if (!a) return NULL;
    CandTensor *res = (CandTensor*)cand_tensor_zeros(a->shape[0], a->shape[1]);
    res->left = a;
    strcpy(res->op, "tanh");
    for (int i = 0; i < a->size; i++) {
        res->data[i] = tanhf(a->data[i]);
    }
    return (void*)res;
}

/* ── Reductions & Stats ───────────────────────────────────────────────────── */

static inline float cand_tensor_sum(void *a_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    if (!a) return 0.0f;
    float s = 0.0f;
    for (int i = 0; i < a->size; i++) s += a->data[i];
    return s;
}

static inline float cand_tensor_mean(void *a_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    if (!a || a->size == 0) return 0.0f;
    return cand_tensor_sum(a) / (float)a->size;
}

static inline int cand_tensor_argmax(void *a_ptr) {
    CandTensor *a = (CandTensor*)a_ptr;
    if (!a || a->size == 0) return 0;
    int idx = 0;
    float max_v = a->data[0];
    for (int i = 1; i < a->size; i++) {
        if (a->data[i] > max_v) {
            max_v = a->data[i];
            idx = i;
        }
    }
    return idx;
}

/* ── Autograd (Reverse-mode Automatic Differentiation) ────────────────────── */

static inline void cand_tensor_zero_grad(void *t_ptr) {
    CandTensor *t = (CandTensor*)t_ptr;
    if (!t) return;
    for (int i = 0; i < t->size; i++) t->grad[i] = 0.0f;
    if (t->left) cand_tensor_zero_grad((void*)t->left);
    if (t->right) cand_tensor_zero_grad((void*)t->right);
}

static inline void _cand_backward_node(CandTensor *t) {
    if (!t) return;

    if (strcmp(t->op, "add") == 0) {
        if (t->left)  for (int i = 0; i < t->left->size;  i++) t->left->grad[i]  += t->grad[i];
        if (t->right) for (int i = 0; i < t->right->size; i++) t->right->grad[i] += t->grad[i];
    } else if (strcmp(t->op, "sub") == 0) {
        if (t->left)  for (int i = 0; i < t->left->size;  i++) t->left->grad[i]  += t->grad[i];
        if (t->right) for (int i = 0; i < t->right->size; i++) t->right->grad[i] -= t->grad[i];
    } else if (strcmp(t->op, "mul") == 0) {
        if (t->left)  for (int i = 0; i < t->left->size;  i++) t->left->grad[i]  += t->grad[i] * t->right->data[i];
        if (t->right) for (int i = 0; i < t->right->size; i++) t->right->grad[i] += t->grad[i] * t->left->data[i];
    } else if (strcmp(t->op, "matmul") == 0) {
        CandTensor *A = t->left;
        CandTensor *B = t->right;
        if (A && B) {
            int M = A->shape[0];
            int K = A->shape[1];
            int N = B->shape[1];
            // dA = dOut * B^T
            for (int i = 0; i < M; i++) {
                for (int k = 0; k < K; k++) {
                    float sum = 0.0f;
                    for (int j = 0; j < N; j++) {
                        sum += t->grad[i * N + j] * B->data[k * N + j];
                    }
                    A->grad[i * K + k] += sum;
                }
            }
            // dB = A^T * dOut
            for (int k = 0; k < K; k++) {
                for (int j = 0; j < N; j++) {
                    float sum = 0.0f;
                    for (int i = 0; i < M; i++) {
                        sum += A->data[i * K + k] * t->grad[i * N + j];
                    }
                    B->grad[k * N + j] += sum;
                }
            }
        }
    } else if (strcmp(t->op, "relu") == 0) {
        if (t->left) {
            for (int i = 0; i < t->left->size; i++) {
                t->left->grad[i] += (t->left->data[i] > 0.0f ? 1.0f : 0.0f) * t->grad[i];
            }
        }
    }

    if (t->left)  _cand_backward_node(t->left);
    if (t->right) _cand_backward_node(t->right);
}

static inline void cand_tensor_backward(void *t_ptr) {
    CandTensor *t = (CandTensor*)t_ptr;
    if (!t) return;
    for (int i = 0; i < t->size; i++) t->grad[i] = 1.0f;
    _cand_backward_node(t);
}

/* ── Neural Network Primitives & Optimizers ──────────────────────────────── */

static inline void* cand_nn_dense_forward(void *input_ptr, void *weights_ptr, void *bias_ptr) {
    CandTensor *out = (CandTensor*)cand_tensor_matmul(input_ptr, weights_ptr);
    CandTensor *bias = (CandTensor*)bias_ptr;
    if (bias && out) {
        for (int r = 0; r < out->shape[0]; r++) {
            for (int c = 0; c < out->shape[1]; c++) {
                out->data[r * out->shape[1] + c] += bias->data[c];
            }
        }
    }
    return (void*)out;
}

static inline void cand_nn_sgd_step(void *weights_ptr, float lr) {
    CandTensor *weights = (CandTensor*)weights_ptr;
    if (!weights) return;
    for (int i = 0; i < weights->size; i++) {
        weights->data[i] -= lr * weights->grad[i];
    }
}

static inline float cand_nn_mse_loss(void *pred_ptr, void *target_ptr) {
    CandTensor *pred = (CandTensor*)pred_ptr;
    CandTensor *target = (CandTensor*)target_ptr;
    if (!pred || !target || pred->size != target->size) return 0.0f;
    float sum_sq = 0.0f;
    for (int i = 0; i < pred->size; i++) {
        float diff = pred->data[i] - target->data[i];
        sum_sq += diff * diff;
    }
    return sum_sq / (float)pred->size;
}

static inline float cand_nn_cross_entropy_loss(void *probs_ptr, int target_class) {
    CandTensor *probs = (CandTensor*)probs_ptr;
    if (!probs || target_class < 0 || target_class >= probs->size) return 0.0f;
    float p = probs->data[target_class];
    if (p < 1e-7f) p = 1e-7f;
    return -logf(p);
}

/* ── Native Tokenizer Engine ─────────────────────────────────────────────── */

typedef struct CandTokenizer {
    char vocab[1024][64];
    int vocab_size;
} CandTokenizer;

static inline CandTokenizer* cand_tokenizer_create(void) {
    CandTokenizer *tok = (CandTokenizer*)calloc(1, sizeof(CandTokenizer));
    return tok;
}

static inline int cand_tokenizer_add_word(CandTokenizer *tok, const char *word) {
    if (!tok || tok->vocab_size >= 1024) return -1;
    strncpy(tok->vocab[tok->vocab_size], word, 63);
    tok->vocab_size++;
    return tok->vocab_size - 1;
}

static inline int cand_tokenizer_lookup(CandTokenizer *tok, const char *word) {
    if (!tok) return -1;
    for (int i = 0; i < tok->vocab_size; i++) {
        if (strcmp(tok->vocab[i], word) == 0) return i;
    }
    return -1;
}

/* ── C& Native Model Serialization (.candmodel) ────────────────────────── */

static inline int cand_model_save(void *t_ptr, const char *filename) {
    CandTensor *t = (CandTensor*)t_ptr;
    if (!t || !filename) return -1;
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
    const char magic[8] = "C&MODEL";
    fwrite(magic, 1, 8, f);
    fwrite(&t->ndim, sizeof(int), 1, f);
    fwrite(t->shape, sizeof(int), 4, f);
    fwrite(&t->size, sizeof(int), 1, f);
    fwrite(t->data, sizeof(float), t->size, f);
    fclose(f);
    return 0;
}

static inline void* cand_model_load(const char *filename) {
    if (!filename) return NULL;
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    char magic[8];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "C&MODEL", 7) != 0) {
        fclose(f);
        return NULL;
    }
    int ndim = 0, size = 0, shape[4] = {0};
    fread(&ndim, sizeof(int), 1, f);
    fread(shape, sizeof(int), 4, f);
    fread(&size, sizeof(int), 1, f);

    CandTensor *t = (CandTensor*)cand_tensor_create(shape[0], shape[1], shape[2], shape[3], ndim);
    fread(t->data, sizeof(float), size, f);
    fclose(f);
    return (void*)t;
}

#ifdef __cplusplus
}
#endif

#endif /* CAND_AIRUNTIME_H */
