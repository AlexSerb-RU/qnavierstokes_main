#include "csr_tools.hpp"

//============================================================================================================
//Неполная LU факторизация матрицы.
//============================================================================================================
void csrtools::partFactorLU(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu,
                            double *l, double *di_f, double *u)
{
    for(int k = 0; k < dimension; k++)
    {
        for(int j = ig[k]; j < ig[k + 1]; j++)
        {
            l[j] = ggl[j];
            u[j] = ggu[j];

            for(int i = ig[k]; i < j; i++)
                for(int m = ig[jg[j]]; m < ig[jg[j] + 1]; m++)
                    if(jg[i] == jg[m])
                    {
                        l[j] -= l[i]*u[m];
                        u[j] -= u[i]*l[m];
                        m = ig[jg[j] + 1];
                    }
                    else
                        if(jg[i] < jg[m])
                            m = ig[jg[j] + 1];

            u[j] = u[j]/di_f[jg[j]];
        }

        di_f[k] = di[k];
        for(int i = ig[k]; i < ig[k + 1]; i++)
            di_f[k] -= l[i]*u[i];
    }
}

//============================================================================================================
//Умножение матрицы на вектор.
//============================================================================================================
void csrtools::matrixMultByVector(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu,
                                  double *vector, double *result)
{
    for(int i = 0; i < dimension; i++)
        result[i] = vector[i]*di[i];

    for(int i = 0; i < dimension; i++)
        for(int j = ig[i]; j < ig[i + 1]; j++)
        {
            result[i] += vector[jg[j]]*ggl[j];
            result[jg[j]] += vector[i]*ggu[j];
        }
}

//============================================================================================================
//Индекс элемента матрицы A[i,j] в массиве ggl.
//============================================================================================================
int csrtools::position(int *ig, int *jg, int i, int j)
{
    int index;
    for(index = ig[i]; jg[index] != j && index < ig[i + 1]; index++)
        ;
    if(index == ig[i + 1])
        return -1;
    return index;
}
