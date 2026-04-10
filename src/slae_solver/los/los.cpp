#include <math.h>
#include <gauss.hpp>
#include <csr_tools.hpp>
#include "los.hpp"

void LOS::solve(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu, double *f,
                double *x, int maxNumOfIter, double sufficientResidual)
{
    double *r;			//¬ектор нев¤зки.
    double *p;			//¬спомогательный вектор метода.
    double *z;			//¬спомогательный вектор метода.
    double *l;			//Ќижн¤ треугольна¤ матрица факторизации без диагонали.
    double *di_f;		//ƒиагональ нижней треугольной матрицы факторизации.
    double *u;			//¬ерхн¤¤ треугольна¤ матрица факторизации.
    double alpha, beta;		//¬спомогательные константы метода.
    double norm_f, norm_r;	//Ќорма правой части, норма нев¤зки.
    double scalar;		//«начение скал¤рного произведени¤ двух векторов.

    double *temp1, *temp2;	//¬ектора дл¤ хранени¤ промежуточных результатов вычислений.

    temp1 = new double[dimension];
    temp2 = new double[dimension];

    l = new double[ig[dimension]];
    u = new double[ig[dimension]];
    di_f = new double[dimension];

    csrtools::partFactorLU(dimension,ig,jg,ggl,di,ggu,l,di_f,u);

    r = new double[dimension];
    p = new double[dimension];
    z = new double[dimension];

    //вычисление нормы f
    norm_f = 0;
    for(int i = 0; i < dimension; i++)
        norm_f += f[i]*f[i];
    norm_f = sqrt(norm_f);

    //нахождение r нуле-вого
    csrtools::matrixMultByVector(dimension,ig,jg,ggl,di,ggu,x,r);
    for(int i = 0; i < dimension; i++)
        r[i] = f[i] - r[i];
    Gauss::solveLowSLAE(dimension,ig,jg,l,di_f,r);

    //вычисление нормы r
    norm_r = 0;
    for(int i = 0; i < dimension; i++)
        norm_r += r[i]*r[i];
    norm_r = sqrt(norm_r);

    //нахождение z нулевого
    for(int i = 0; i < dimension; i++)
        z[i] = r[i];
    Gauss::solveUpSLAE(dimension,ig,jg,u,z);

    //нахождение p нуле-вого
    csrtools::matrixMultByVector(dimension,ig,jg,ggl,di,ggu,z,p);
    Gauss::solveLowSLAE(dimension,ig,jg,l,di_f,p);

    //итерационный шаг
    for(int curIter = 0; curIter < maxNumOfIter && norm_r/norm_f > sufficientResidual; curIter++)
    {
        //нахождение alpha i-го
        alpha = p[0]*r[0];
        scalar = p[0]*p[0];
        for(int i = 1; i < dimension; i++)
        {
            alpha += p[i]*r[i];
            scalar += p[i]*p[i];
        }
        alpha /= scalar;

        //нахождение x i-го
        for(int i = 0; i < dimension; i++)
            x[i] += alpha*z[i];

        //нахождение r i-го
        for(int i = 0; i < dimension; i++)
            r[i] -= alpha*p[i];

        //вспомогательный вектор дл¤ нахождени¤ beta
        for(int i = 0; i < dimension; i++)
            temp1[i] = r[i];
        Gauss::solveUpSLAE(dimension,ig,jg,u,temp1);
        csrtools::matrixMultByVector(dimension,ig,jg,ggl,di,ggu,temp1,temp2);
        Gauss::solveLowSLAE(dimension,ig,jg,l,di_f,temp2);

        //нахождение beta i-го
        beta = p[0]*temp2[0];
        for(int i = 1; i < dimension; i++)
            beta += p[i]*temp2[i];
        beta /= -scalar;

        //нахождение p i-го
        for(int i = 0; i < dimension; i++)
        {
            p[i] *= beta;
            p[i] += temp2[i];
        }

        //вспомогательный вектор дл¤ нахождени¤ z i-го
        for(int i = 0; i < dimension; i++)
            temp1[i] = r[i];
        Gauss::solveUpSLAE(dimension,ig,jg,u,temp1);

        //нахождение z i-го
        for(int i = 0; i < dimension; i++)
        {
            z[i] *= beta;
            z[i] += temp1[i];
        }

        //вычисление нормы r
        norm_r = 0;
        for(int i = 0; i < dimension; i++)
            norm_r += r[i]*r[i];
        norm_r = sqrt(norm_r);
    }

    delete [] temp1;
    delete [] temp2;
    delete [] r;
    delete [] p;
    delete [] z;
    delete [] l;
    delete [] u;
    delete [] di_f;
}
