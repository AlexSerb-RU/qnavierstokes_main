#include <math.h>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QApplication>
#include "side_heating_solver.hpp"
#include <los.hpp>
#include <csr_tools.hpp>
#include <triangle_fem_tools.hpp>

//============================================================================================================
//============================================================================================================
SideHeatingSolver::SideHeatingSolver(QObject *parent) : QThread(parent)
{
    xy = NULL;
    nvtr = NULL;

    T_bc1 = NULL;
    topT_bc1 = NULL;
    topPsiOmega_bc1 = NULL;

    ig = NULL;
    jg = NULL;
    ggl = NULL;
    di = NULL;
    ggu = NULL;
    f = NULL;

    t = NULL;
    psi = NULL;
    omega = NULL;

    tOld = NULL;
    psiOld = NULL;
    omegaOld = NULL;

    tx = NULL;
    vx = NULL;
    vy = NULL;

    isSolutionCutOff = false;
}

//============================================================================================================
//Прервать работу решателя.
//============================================================================================================
void SideHeatingSolver::cutOffSolution()
{
    mutex.lock();
    isSolutionCutOff = true;
    mutex.unlock();
}

//============================================================================================================
//Функция для задания параметров задачи.
//============================================================================================================
void SideHeatingSolver::setProblemParameters(double height, double Pr, double Gr, bool isLeftWallHard,
                                             bool isRightWallHard, bool isTopWallHard, bool isBottomWallHard)
{
    this->width = 1;		//В безразмерных координатах ширина расчетной области равна 1
    this->height = height;
    this->Pr = Pr;
    this->Gr = Gr;
    this->isLeftWallHard = isLeftWallHard;
    this->isRightWallHard = isRightWallHard;
    this->isTopWallHard = isTopWallHard;
    this->isBottomWallHard = isBottomWallHard;
}

//============================================================================================================
//Функция для задания параметров решателя
//============================================================================================================
void SideHeatingSolver::setSolverParameters(int nx, int ny, int maxNumOfIter,
                                            double wT, double wPsi, double wOmega)
{
        this->nx = nx;
        this->ny = ny;
        this->maxNumOfIter = maxNumOfIter;
        this->wT = wT;
        this->wPsi = wPsi;
        this->wOmega = wOmega;
}

//============================================================================================================
//Функция, которая будет выполняться в отдельном потоке. В данном случае - эта функция отвечает за
//конечноэлементное решение поставленной задачи.
//============================================================================================================
void SideHeatingSolver::run()
{
    prepareData();

    mutex.lock();
    isSolutionCutOff = false;

    int currentIter = 0;
    do
    {
        mutex.unlock();

        for(int i = 0; i < numOfPoints; i++)
        {
            tOld[i] = t[i];
            psiOld[i] = psi[i];
            omegaOld[i] = omega[i];
        }

        formV();

        //решаем уравнение для T
        clearSLAE();
        tSLAE();
        LOS::solve(numOfPoints,ig,jg,ggl,di,ggu,f,t);
        for(int i = 0; i < numOfPoints; i++)
            t[i] = (1 - wT)*tOld[i] + wT*t[i];

        //Решаем уравнение для Omega
        clearSLAE();
        omegaSLAE();
        LOS::solve(numOfPoints,ig,jg,ggl,di,ggu,f,omega);
        for(int i = 0; i < numOfPoints; i++)
            omega[i] = (1 - wOmega)*omegaOld[i] + wOmega*omega[i];

        //решаем уравнение для Psi
        clearSLAE();
        psiSLAE();
        LOS::solve(numOfPoints,ig,jg,ggl,di,ggu,f,psi);
        for(int i = 0; i < numOfPoints; i++)
            psi[i] = (1 - wPsi)*psiOld[i] + wPsi*psi[i];

                emit iterationFinished(currentIter,maxNumOfIter,residual());

        mutex.lock();
    }while(++currentIter < maxNumOfIter && !isSolutionCutOff && residual() > 1e-6);

    if(currentIter >= maxNumOfIter && !isSolutionCutOff)
        emit maxIterNumberAttained(residual());
    if(!isSolutionCutOff)
    {
        output();
        emit solutionFinished();
    }
    mutex.unlock();

    freeMemory();
}

//============================================================================================================
//Функция выделяет динамическую память для данных, создает конечноэлементную сетку, портрет матрицы СЛАУ,
//а также задает начальное приближение для искомых функций T, Psi и Omega.
//============================================================================================================
void SideHeatingSolver::prepareData()
{
    hx = width/(nx - 1);
    hy = height/(ny - 1);

    numOfPoints = nx*ny;
    numOfTriangles = (nx - 1)*(ny - 1)*2;

    //********************************************************************************************************
    //Создание сетки.
    //********************************************************************************************************
    nvtr = new int*[numOfTriangles];
    for(int i = 0; i < numOfTriangles; i++)
        nvtr[i] = new int[4];

    xy = new double*[numOfPoints];
    for(int i = 0; i < numOfPoints; i++)
        xy[i] = new double[2];

    for(int i = 0; i < ny - 1; i++)
        for(int j = 0; j < nx - 1; j++)
        {
            nvtr[2*((nx - 1)*i + j)][0] = nx*i + j;
            nvtr[2*((nx - 1)*i + j)][1] = nx*i + j + 1;
            nvtr[2*((nx - 1)*i + j)][2] = nx*(i + 1) + j;
            nvtr[2*((nx - 1)*i + j)][3] = 1;

            nvtr[2*((nx - 1)*i + j) + 1][0] = nx*(i + 1) + j;
            nvtr[2*((nx - 1)*i + j) + 1][1] = nx*(i + 1) + j + 1;
            nvtr[2*((nx - 1)*i + j) + 1][2] = nx*i + j + 1;
            nvtr[2*((nx - 1)*i + j) + 1][3] = 1;
        }

    for(int i = 0; i < ny; i++)
        for(int j = 0; j < nx; j++)
        {
            xy[nx*i + j][0] = j/(nx - 1.);
            xy[nx*i + j][1] = i*height/(ny - 1.);
        }

    //********************************************************************************************************
    //Создание информации о краевых условиях.
    //********************************************************************************************************
    numOfPointsT_bc1 = 2*ny;
    numOfPointsPsiOmega_bc1 = 2*(nx + ny) - 4;

    T_bc1 = new double[numOfPointsT_bc1];
    topT_bc1 = new int[numOfPointsT_bc1];
    topPsiOmega_bc1 = new int[numOfPointsPsiOmega_bc1];

    for(int i = 0; i < ny; i++)
    {
        topT_bc1[i] = nx*i;
        topT_bc1[ny + i] = nx*(i + 1) - 1;
        T_bc1[i] = 0;
        T_bc1[ny + i] = 1;
    }

    for(int i = 0; i < ny; i++)
    {
        topPsiOmega_bc1[i] = nx*i;
        topPsiOmega_bc1[ny + i] = nx*(i + 1) - 1;
    }

    for(int i = 1; i < nx - 1; i++)
    {
        topPsiOmega_bc1[2*ny + i - 1] = i;
        topPsiOmega_bc1[2*ny + nx - 3 + i] = (ny - 1)*nx + i;
    }

    //********************************************************************************************************
    //Формирование матрицы СЛАУ, выделение памяти под вектор правой части СЛАУ и вектора неизвестных.
    //Задание начальных приближений векторов неизвестных.
    //********************************************************************************************************
    trianglefemtools::formMatrixProfile(numOfTriangles,numOfPoints,nvtr,&ig,&jg);

    ggl = new double[ig[numOfPoints]];
    di = new double[numOfPoints];
    ggu = new double[ig[numOfPoints]];
    f = new double[numOfPoints];

    t = new double[numOfPoints];
    psi = new double[numOfPoints];
    omega = new double[numOfPoints];

    tOld = new double[numOfPoints];
    psiOld = new double[numOfPoints];
    omegaOld = new double[numOfPoints];

    tx = new double[numOfPoints];
    vx = new double[numOfPoints];
    vy = new double[numOfPoints];

    for(int i = 0; i < numOfPoints; i++)
    {
        t[i] = xy[i][0];
        psi[i] = 0;
        omega[i] = 0;
    }
}

//============================================================================================================
//Освобождение динамически выделенной памяти.
//============================================================================================================
void SideHeatingSolver::freeMemory()
{
    for(int i = 0; i < numOfPoints; i++)
        delete [] xy[i];
    delete [] xy;
    xy = NULL;

    for(int i = 0; i < numOfTriangles; i++)
        delete [] nvtr[i];
    delete [] nvtr;
    nvtr = NULL;

    delete [] T_bc1;
    delete [] topT_bc1;
    delete [] topPsiOmega_bc1;
    T_bc1 = NULL;
    topT_bc1 = NULL;
    topPsiOmega_bc1 = NULL;

    delete [] ig;
    delete [] jg;
    delete [] ggl;
    delete [] di;
    delete [] ggu;
    delete [] f;
    ig = NULL;
    jg = NULL;
    ggl = NULL;
    di = NULL;
    ggu = NULL;
    f = NULL;

    delete [] t;
    delete [] psi;
    delete [] omega;
    t = NULL;
    psi = NULL;
    omega = NULL;

    delete [] tOld;
    delete [] psiOld;
    delete [] omegaOld;
    tOld = NULL;
    psiOld = NULL;
    omegaOld = NULL;

    delete [] tx;
    delete [] vx;
    delete [] vy;
    tx = NULL;
    vx = NULL;
    vy = NULL;
}

//============================================================================================================
//Вывод результатов.
//============================================================================================================
void SideHeatingSolver::output()
{
    QFile file;
    QTextStream ostream(&file);
    ostream.setRealNumberPrecision(13);

    //********************************************************************************************************
    //выводим информацию о сетке.
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/nvtr.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < numOfTriangles; i++)
        ostream << nvtr[i][0] << " " << nvtr[i][1] << " " << nvtr[i][2] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения функции температуры
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/T.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < numOfPoints; i++)
        ostream << xy[i][0] << " " << xy[i][1] << " " << t[i] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения функции тока
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/Psi.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < numOfPoints; i++)
        ostream << xy[i][0] << " " << xy[i][1] << " " << psi[i] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения вихря
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/Omega.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < numOfPoints; i++)
        ostream << xy[i][0] << " " << xy[i][1] << " " << omega[i] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения x-составляющей функции скорости
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/Vx.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < numOfPoints; i++)
        ostream << xy[i][0] << " " << xy[i][1] << " " << vx[i] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения y-составляющей функции скорости
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/Vy.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < numOfPoints; i++)
        ostream << xy[i][0] << " " << xy[i][1] << " " << vy[i] << Qt::endl;
    file.close();
}

//============================================================================================================
//Расчет невязки по нелинейности с учетом коэффициента релаксации.
//============================================================================================================
double SideHeatingSolver::residual()
{
    double epsT(0);
    double normT(0);
    double epsPsi(0);
    double normPsi(0);
    double epsOmega(0);
    double normOmega(0);

    for(int i = 0; i < numOfPoints; i++)
    {
        normT += t[i]*t[i];
        epsT += (t[i] - tOld[i])*(t[i] - tOld[i]);
        normPsi += psi[i]*psi[i];
        epsPsi += (psi[i] - psiOld[i])*(psi[i] - psiOld[i]);
        normOmega += omega[i]*omega[i];
        epsOmega += (omega[i] - omegaOld[i])*(omega[i] - omegaOld[i]);
    }
    return qMax(sqrt(epsT/normT)/wT,qMax(sqrt(epsPsi/normPsi)/wPsi,sqrt(epsOmega/normOmega)/wOmega));
}

//============================================================================================================
//Очистка матрицы СЛАУ и вектора правой части(заполнение их нулями).
//============================================================================================================
void SideHeatingSolver::clearSLAE()
{
    for(int i = 0; i < numOfPoints; i++)
    {
        di[i] = 0;
        f[i] = 0;
    }

    for(int i = 0; i < ig[numOfPoints]; i++)
    {
        ggl[i] = 0;
        ggu[i] = 0;
    }
}

//============================================================================================================
//============================================================================================================
void SideHeatingSolver::tSLAE()
{
    //делаем конечноэлементную сборку
    for(int currentTriangle = 0; currentTriangle < numOfTriangles; currentTriangle++)
    {
        double D[3][3];		//Вспомогательная матрица для определения коэффициентов базисных функций
        double C[3][3];		//Локальная матрица массы
        double detD;		//Определитель матрицы D

        detD = (xy[nvtr[currentTriangle][1]][0] - xy[nvtr[currentTriangle][0]][0])*
               (xy[nvtr[currentTriangle][2]][1] - xy[nvtr[currentTriangle][0]][1])-
               (xy[nvtr[currentTriangle][2]][0] - xy[nvtr[currentTriangle][0]][0])*
               (xy[nvtr[currentTriangle][1]][1] - xy[nvtr[currentTriangle][0]][1]);
        D[0][0] = (xy[nvtr[currentTriangle][1]][0]*xy[nvtr[currentTriangle][2]][1]-
                   xy[nvtr[currentTriangle][2]][0]*xy[nvtr[currentTriangle][1]][1])/detD;
        D[0][1] = (xy[nvtr[currentTriangle][1]][1] - xy[nvtr[currentTriangle][2]][1])/detD;
        D[0][2] = (xy[nvtr[currentTriangle][2]][0] - xy[nvtr[currentTriangle][1]][0])/detD;
        D[1][0] = (xy[nvtr[currentTriangle][2]][0]*xy[nvtr[currentTriangle][0]][1]-
                   xy[nvtr[currentTriangle][0]][0]*xy[nvtr[currentTriangle][2]][1])/detD;
        D[1][1] = (xy[nvtr[currentTriangle][2]][1] - xy[nvtr[currentTriangle][0]][1])/detD;
        D[1][2] = (xy[nvtr[currentTriangle][0]][0] - xy[nvtr[currentTriangle][2]][0])/detD;
        D[2][0] = (xy[nvtr[currentTriangle][0]][0]*xy[nvtr[currentTriangle][1]][1]-
                   xy[nvtr[currentTriangle][1]][0]*xy[nvtr[currentTriangle][0]][1])/detD;
        D[2][1] = (xy[nvtr[currentTriangle][0]][1] - xy[nvtr[currentTriangle][1]][1])/detD;
        D[2][2] = (xy[nvtr[currentTriangle][1]][0] - xy[nvtr[currentTriangle][0]][0])/detD;

        C[0][0] = fabs(detD)/12;	C[0][1] = fabs(detD)/24;	C[0][2] = fabs(detD)/24;
        C[1][0] = fabs(detD)/24;	C[1][1] = fabs(detD)/12;	C[1][2] = fabs(detD)/24;
        C[2][0] = fabs(detD)/24;	C[2][1] = fabs(detD)/24;	C[2][2] = fabs(detD)/12;

        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
            {
                double addition = (D[i][1]*D[j][1] + D[i][2]*D[j][2])*fabs(detD)/(2*Pr);
                addition += (vx[nvtr[currentTriangle][0]]*C[i][0] + vx[nvtr[currentTriangle][1]]*C[i][1] +
                             vx[nvtr[currentTriangle][2]]*C[i][2])*D[j][1];
                addition += (vy[nvtr[currentTriangle][0]]*C[i][0] + vy[nvtr[currentTriangle][1]]*C[i][1] +
                             vy[nvtr[currentTriangle][2]]*C[i][2])*D[j][2];

                if(nvtr[currentTriangle][i] == nvtr[currentTriangle][j])
                    di[nvtr[currentTriangle][i]] += addition;
                else if(nvtr[currentTriangle][i] > nvtr[currentTriangle][j])
                    ggl[csrtools::position(ig,jg,nvtr[currentTriangle][i],nvtr[currentTriangle][j])] += addition;
                else
                    ggu[csrtools::position(ig,jg,nvtr[currentTriangle][j],nvtr[currentTriangle][i])] += addition;
            }
        }

    //учитываем краевые условия первого рода
    for(int i = 0; i < numOfPointsT_bc1; i++)
    {
        di[topT_bc1[i]] = 1;
        f[topT_bc1[i]] = T_bc1[i];
        for(int j = ig[topT_bc1[i]]; j < ig[topT_bc1[i] + 1]; j++)
            ggl[j] = 0;
        for(int j = 0; j < ig[numOfPoints]; j++)
            if(jg[j] == topT_bc1[i])
                ggu[j] = 0;
    }
}

//============================================================================================================
//============================================================================================================
void SideHeatingSolver::psiSLAE()
{
    //делаем конечноэлементную сборку
    for(int currentTriangle = 0; currentTriangle < numOfTriangles; currentTriangle++)
    {
        double D[3][3];		//Вспомогательная матрица для определения коэффициентов базисных функций
        double detD;		//Определитель матрицы D

        detD = (xy[nvtr[currentTriangle][1]][0] - xy[nvtr[currentTriangle][0]][0])*
               (xy[nvtr[currentTriangle][2]][1] - xy[nvtr[currentTriangle][0]][1])-
               (xy[nvtr[currentTriangle][2]][0] - xy[nvtr[currentTriangle][0]][0])*
               (xy[nvtr[currentTriangle][1]][1] - xy[nvtr[currentTriangle][0]][1]);
        D[0][0] = (xy[nvtr[currentTriangle][1]][0]*xy[nvtr[currentTriangle][2]][1]-
                   xy[nvtr[currentTriangle][2]][0]*xy[nvtr[currentTriangle][1]][1])/detD;
        D[0][1] = (xy[nvtr[currentTriangle][1]][1] - xy[nvtr[currentTriangle][2]][1])/detD;
        D[0][2] = (xy[nvtr[currentTriangle][2]][0] - xy[nvtr[currentTriangle][1]][0])/detD;
        D[1][0] = (xy[nvtr[currentTriangle][2]][0]*xy[nvtr[currentTriangle][0]][1]-
                   xy[nvtr[currentTriangle][0]][0]*xy[nvtr[currentTriangle][2]][1])/detD;
        D[1][1] = (xy[nvtr[currentTriangle][2]][1] - xy[nvtr[currentTriangle][0]][1])/detD;
        D[1][2] = (xy[nvtr[currentTriangle][0]][0] - xy[nvtr[currentTriangle][2]][0])/detD;
        D[2][0] = (xy[nvtr[currentTriangle][0]][0]*xy[nvtr[currentTriangle][1]][1]-
                   xy[nvtr[currentTriangle][1]][0]*xy[nvtr[currentTriangle][0]][1])/detD;
        D[2][1] = (xy[nvtr[currentTriangle][0]][1] - xy[nvtr[currentTriangle][1]][1])/detD;
        D[2][2] = (xy[nvtr[currentTriangle][1]][0] - xy[nvtr[currentTriangle][0]][0])/detD;

        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
            {
                double addition = (D[i][1]*D[j][1] + D[i][2]*D[j][2])*fabs(detD)/2;
                if(nvtr[currentTriangle][i] == nvtr[currentTriangle][j])
                    di[nvtr[currentTriangle][i]] += addition;
                else if(nvtr[currentTriangle][i] > nvtr[currentTriangle][j])
                    ggl[csrtools::position(ig,jg,nvtr[currentTriangle][i],nvtr[currentTriangle][j])] += addition;
                else
                    ggu[csrtools::position(ig,jg,nvtr[currentTriangle][j],nvtr[currentTriangle][i])] += addition;
            }

        f[nvtr[currentTriangle][0]] += (2*omega[nvtr[currentTriangle][0]] + omega[nvtr[currentTriangle][1]] +
                                        omega[nvtr[currentTriangle][2]])*fabs(detD)/24.;
        f[nvtr[currentTriangle][1]] += (omega[nvtr[currentTriangle][0]] + 2*omega[nvtr[currentTriangle][1]] +
                                        omega[nvtr[currentTriangle][2]])*fabs(detD)/24.;
        f[nvtr[currentTriangle][2]] += (omega[nvtr[currentTriangle][0]] + omega[nvtr[currentTriangle][1]] +
                                        2*omega[nvtr[currentTriangle][2]])*fabs(detD)/24.;
    }

    //учитываем краевые условия первого рода
    for(int i = 0; i < numOfPointsPsiOmega_bc1; i++)
    {
        di[topPsiOmega_bc1[i]] = 1;
        f[topPsiOmega_bc1[i]] = 0;
        for(int j = ig[topPsiOmega_bc1[i]]; j < ig[topPsiOmega_bc1[i] + 1]; j++)
            ggl[j] = 0;
        for(int j = 0; j < ig[numOfPoints]; j++)
            if(jg[j] == topPsiOmega_bc1[i])
                ggu[j] = 0;
    }
}

//============================================================================================================
//============================================================================================================
void SideHeatingSolver::omegaSLAE()
{
    for(int i = 0; i < ny; i++)
    {
        for(int j = 0; j < nx - 1; j++)
            tx[i*nx + j] = (t[i*nx + j + 1] - t[i*nx + j])/hx;
        tx[i*nx + nx - 1] = tx[i*nx + nx - 2];
    }

    //делаем конечноэлементную сборку
    for(int currentTriangle = 0; currentTriangle < numOfTriangles; currentTriangle++)
    {
        double D[3][3];		//Вспомогательная матрица для определения коэффициентов базисных функций
        double C[3][3];		//Локальная матрица массы
        double detD;		//Определитель матрицы D

        detD = (xy[nvtr[currentTriangle][1]][0] - xy[nvtr[currentTriangle][0]][0])*
               (xy[nvtr[currentTriangle][2]][1] - xy[nvtr[currentTriangle][0]][1])-
               (xy[nvtr[currentTriangle][2]][0] - xy[nvtr[currentTriangle][0]][0])*
               (xy[nvtr[currentTriangle][1]][1] - xy[nvtr[currentTriangle][0]][1]);
        D[0][0] = (xy[nvtr[currentTriangle][1]][0]*xy[nvtr[currentTriangle][2]][1]-
                   xy[nvtr[currentTriangle][2]][0]*xy[nvtr[currentTriangle][1]][1])/detD;
        D[0][1] = (xy[nvtr[currentTriangle][1]][1] - xy[nvtr[currentTriangle][2]][1])/detD;
        D[0][2] = (xy[nvtr[currentTriangle][2]][0] - xy[nvtr[currentTriangle][1]][0])/detD;
        D[1][0] = (xy[nvtr[currentTriangle][2]][0]*xy[nvtr[currentTriangle][0]][1]-
                   xy[nvtr[currentTriangle][0]][0]*xy[nvtr[currentTriangle][2]][1])/detD;
        D[1][1] = (xy[nvtr[currentTriangle][2]][1] - xy[nvtr[currentTriangle][0]][1])/detD;
        D[1][2] = (xy[nvtr[currentTriangle][0]][0] - xy[nvtr[currentTriangle][2]][0])/detD;
        D[2][0] = (xy[nvtr[currentTriangle][0]][0]*xy[nvtr[currentTriangle][1]][1]-
                   xy[nvtr[currentTriangle][1]][0]*xy[nvtr[currentTriangle][0]][1])/detD;
        D[2][1] = (xy[nvtr[currentTriangle][0]][1] - xy[nvtr[currentTriangle][1]][1])/detD;
        D[2][2] = (xy[nvtr[currentTriangle][1]][0] - xy[nvtr[currentTriangle][0]][0])/detD;

        C[0][0] = fabs(detD)/12;	C[0][1] = fabs(detD)/24;	C[0][2] = fabs(detD)/24;
        C[1][0] = fabs(detD)/24;	C[1][1] = fabs(detD)/12;	C[1][2] = fabs(detD)/24;
        C[2][0] = fabs(detD)/24;	C[2][1] = fabs(detD)/24;	C[2][2] = fabs(detD)/12;

        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
            {
                double addition = (D[i][1]*D[j][1] + D[i][2]*D[j][2])*fabs(detD)/2;
                addition += (vx[nvtr[currentTriangle][0]]*C[i][0] + vx[nvtr[currentTriangle][1]]*C[i][1] +
                             vx[nvtr[currentTriangle][2]]*C[i][2])*D[j][1];
                addition += (vy[nvtr[currentTriangle][0]]*C[i][0] + vy[nvtr[currentTriangle][1]]*C[i][1] +
                             vy[nvtr[currentTriangle][2]]*C[i][2])*D[j][2];

                if(nvtr[currentTriangle][i] == nvtr[currentTriangle][j])
                    di[nvtr[currentTriangle][i]] += addition;
                else if(nvtr[currentTriangle][i] > nvtr[currentTriangle][j])
                    ggl[csrtools::position(ig,jg,nvtr[currentTriangle][i],nvtr[currentTriangle][j])] += addition;
                else
                    ggu[csrtools::position(ig,jg,nvtr[currentTriangle][j],nvtr[currentTriangle][i])] += addition;
            }

        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                f[nvtr[currentTriangle][i]] += tx[nvtr[currentTriangle][j]]*C[i][j]*Gr;
    }

    //учитываем краевые условия первого рода
    for(int i = 0; i < numOfPointsPsiOmega_bc1; i++)
    {
        di[topPsiOmega_bc1[i]] = 1;
        if(topPsiOmega_bc1[i] == 0 || topPsiOmega_bc1[i] == nx - 1 || topPsiOmega_bc1[i] == (ny - 1)*nx || topPsiOmega_bc1[i] == ny*nx - 1)
            f[topPsiOmega_bc1[i]] = 0;
        else
            if(topPsiOmega_bc1[i] > 0 && topPsiOmega_bc1[i] < nx - 1)
            {
                if(isBottomWallHard)
                    f[topPsiOmega_bc1[i]] = (1 - wOmega)*omegaOld[topPsiOmega_bc1[i]] +
                                            wOmega*(-omegaOld[topPsiOmega_bc1[i] + nx]/2. - 3.*psi[topPsiOmega_bc1[i] + nx]/(hy*hy));
                else
                    f[topPsiOmega_bc1[i]] = 0;
            }
            else
                if(topPsiOmega_bc1[i] > nx*(ny - 1) && topPsiOmega_bc1[i] < ny*nx - 1)
                {
                    if(isTopWallHard)
                        f[topPsiOmega_bc1[i]] = (1 - wOmega)*omegaOld[topPsiOmega_bc1[i]] +
                                                wOmega*(-omegaOld[topPsiOmega_bc1[i] - nx]/2. - 3.*psi[topPsiOmega_bc1[i] - nx]/(hy*hy));
                    else
                        f[topPsiOmega_bc1[i]] = 0;
                }
                else
                    if(topPsiOmega_bc1[i]%nx == 0)
                    {
                        if(isLeftWallHard)
                            f[topPsiOmega_bc1[i]] = (1 - wOmega)*omegaOld[topPsiOmega_bc1[i]] +
                                                    wOmega*(-omegaOld[topPsiOmega_bc1[i] + 1]/2. - 3.*psi[topPsiOmega_bc1[i] + 1]/(hx*hx));
                        else
                            f[topPsiOmega_bc1[i]] = 0;
                    }
                    else
                    {
                        if(isRightWallHard)
                            f[topPsiOmega_bc1[i]] = (1 - wOmega)*omegaOld[topPsiOmega_bc1[i]] +
                                                    wOmega*(-omegaOld[topPsiOmega_bc1[i] - 1]/2. - 3.*psi[topPsiOmega_bc1[i] - 1]/(hx*hx));
                        else
                            f[topPsiOmega_bc1[i]] = 0;
                    }

            for(int j = ig[topPsiOmega_bc1[i]]; j < ig[topPsiOmega_bc1[i] + 1]; j++)
                ggl[j] = 0;
            for(int j = 0; j < ig[numOfPoints]; j++)
                if(jg[j] == topPsiOmega_bc1[i])
                    ggu[j] = 0;
        }
}

//============================================================================================================
//Вычисление вектора скорости.
//============================================================================================================
void SideHeatingSolver::formV()
{
    //считаем x-компоненту скорсти
    for(int i = 0; i < (ny - 1)*nx; i++)
        vx[i] = (psi[i + nx] - psi[i])/hy;
    for(int i = (ny - 1)*nx; i < ny*nx; i++)
        vx[i] = vx[i - nx];

    //считаем y-компоненту скорости
    for(int i = 0; i < ny; i++)
    {
        for(int j = 0; j < nx - 1; j++)
            vy[i*nx + j] = (psi[i*nx + j] - psi[i*nx + j + 1])/hx;
        vy[i*nx + nx - 1] = vy[i*nx + nx - 2];
    }

    if(isBottomWallHard)
        for(int i = 0; i < nx; i++)
            vx[i] = 0;

    if(isTopWallHard)
        for(int i = (ny - 1)*nx; i < ny*nx; i++)
            vx[i] = 0;

    if(isLeftWallHard)
        for(int i = 0; i < ny; i++)
            vy[i*nx] = 0;

    if(isRightWallHard)
        for(int i = 0; i < ny; i++)
            vy[i*nx + nx - 1] = 0;
}

//============================================================================================================
//============================================================================================================
double SideHeatingSolver::getDefaultWT(){return 0.3;}
double SideHeatingSolver::getDefaultWPsi(){return 0.3;}
double SideHeatingSolver::getDefaultWOmega(){return 0.3;}
int SideHeatingSolver::getDefaultMaxNumOfIter(){return 300;}

//============================================================================================================
//============================================================================================================
SideHeatingSolver::~SideHeatingSolver()
{
    if(xy != NULL)
    {
        for(int i = 0; i < numOfPoints; i++)
            delete [] xy[i];
        delete [] xy;
    }

    if(nvtr != NULL)
    {
        for(int i = 0; i < numOfTriangles; i++)
            delete [] nvtr[i];
        delete [] nvtr;
    }

    if(T_bc1 != NULL) delete [] T_bc1;
    if(topT_bc1 != NULL) delete [] topT_bc1;
    if(topPsiOmega_bc1 != NULL) delete [] topPsiOmega_bc1;

    if(ig != NULL) delete [] ig;
    if(jg != NULL) delete [] jg;
    if(ggl != NULL) delete [] ggl;
    if(di != NULL) delete [] di;
    if(ggu != NULL) delete [] ggu;
    if(f != NULL) delete [] f;

    if(t != NULL) delete [] t;
    if(psi != NULL) delete [] psi;
    if(omega != NULL) delete [] omega;

    if(tOld != NULL) delete [] tOld;
    if(psiOld != NULL) delete [] psiOld;
    if(omegaOld != NULL) delete [] omegaOld;

    if(tx != NULL) delete [] tx;
    if(vx != NULL) delete [] vx;
    if(vy != NULL) delete [] vy;
}
