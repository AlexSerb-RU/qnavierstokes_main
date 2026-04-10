#include <math.h>
#include <stdlib.h>
#include <QtCore/QFile>
#include <QtCore/QRandomGenerator>
#include <QtCore/QTextStream>
#include <QtWidgets/QApplication>
#include "bottom_heating_solver.hpp"

//============================================================================================================
//============================================================================================================
BottomHeatingSolver::BottomHeatingSolver(QObject *parent) : QThread(parent)
{
    x = NULL;
    y = NULL;

    t = NULL;
    vx = NULL;
    vy = NULL;
    psi = NULL;
    omega = NULL;

    isSolutionCutOff = false;
}

//============================================================================================================
//Прервать работу решателя.
//============================================================================================================
void BottomHeatingSolver::cutOffSolution()
{
    mutex.lock();
    isSolutionCutOff = true;
    mutex.unlock();
}

//============================================================================================================
//Функция для задания параметров задачи.
//============================================================================================================
void BottomHeatingSolver::setProblemParameters(double width, double Pr, double Gr, bool isLeftWallHard,
                                               bool isRightWallHard, bool isTopWallHard, bool isBottomWallHard)
{
    this->width = width;
    this->height = 1.0;		//В безразмерных координатах высота расчетной области равна 1
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
void BottomHeatingSolver::setSolverParameters(int nx, int ny, int maxNumOfIter,
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
//конечноразностное решение поставленной задачи.
//============================================================================================================
void BottomHeatingSolver::run()
{
    prepareData();

    double normEpsT;     //Максимальная разница между соответствующими значениями температуры на разных итерациях
    double normEpsPsi;   //Максимальная разница между соответствующими значениями функции тока на разных итерациях
    double normEpsOmega; //Максимальная разница между соответствующими значениями вихря на разных итерациях

    double tNew, psiNew, omegaNew;  //Новое значение соответствующей функции в обрабатываемой точке
    double A1, A2, B1, B2, C;       //переменные для упрощения внешнего вида формул
    double u1, u2, v1, v2;          //переменные для упрощения внешнего вида формул

    mutex.lock();
    isSolutionCutOff = false;

    int currentIter = 0;
    do
    {
        mutex.unlock();

        normEpsT = 0.0;
        normEpsPsi = 0.0;
        normEpsOmega = 0.0;

        //краевые условия на нижней границе
        for(int j = 0; j < nx; j++)
        {
            t[0][j] = 1;
            psi[0][j] = 0.0;
            if(isBottomWallHard)
                omega[0][j] = (1 - wOmega)*omega[0][j] +
                              wOmega*(-0.5*omega[1][j] - 3.0*psi[1][j]/(hy*hy));
            else
                omega[0][j] = 0;
        }

        //краевые условия в угловых точках
        omega[0][0] = 0.0;
        omega[ny - 1][0] = 0.0;

        for(int i = 1; i < ny - 1; i++)
        {
            //краевые условия слева
            t[i][0] = 1.0 - i/(ny - 1.0);
            psi[i][0]=0.0;

            if(isLeftWallHard)
                omega[i][0] = (1 - wOmega)*omega[i][0] +
                              wOmega*(-0.5*omega[i][1] - 3.0*psi[i][1]/(hx*hx));
            else
                omega[i][0] = 0;

            for(int j = 1; j < nx - 1; j++)
            {
                //находим новое приближение решения для функции температуры
                u1 = (psi[i + 1][j - 1] - psi[i - 1][j - 1] + psi[i + 1][j] - psi[i - 1][j])/(4.*hy);
                u2 = (psi[i + 1][j + 1] - psi[i - 1][j + 1] + psi[i + 1][j] - psi[i - 1][j])/(4.*hy);
                v1 = (psi[i - 1][j - 1] - psi[i - 1][j + 1] + psi[i][j - 1] - psi[i][j + 1])/(4.*hx);
                v2 = (psi[i + 1][j - 1] - psi[i + 1][j + 1] + psi[i][j - 1] - psi[i][j + 1])/(4.*hx);

                A1 = (2./(2. + qAbs(u1)*Pr*hx) + (u1 + qAbs(u1))*Pr*hx*0.5)/(hx*hx);
                A2 = (2./(2. + qAbs(v1)*Pr*hy) + (v1 + qAbs(v1))*Pr*hy*0.5)/(hy*hy);
                B1 = (2./(2. + qAbs(u2)*Pr*hx) - (u2 - qAbs(u2))*Pr*hx*0.5)/(hx*hx);
                B2 = (2./(2. + qAbs(v2)*Pr*hy) - (v2 - qAbs(v2))*Pr*hy*0.5)/(hy*hy);
                C = A1 + A2 + B1 + B2;

                tNew = (1 - wT)*t[i][j] + wT*(A1*t[i][j - 1] + A2*t[i - 1][j] +
                                              B1*t[i][j + 1] + B2*t[i + 1][j])/C;
                normEpsT += (tNew - t[i][j])*(tNew - t[i][j]);
                t[i][j] = tNew;

                //--------------------------------------------------------------------------------------------
                //--------------------------------------------------------------------------------------------
                //находим новое приближение решения для функции вихря
                A1 = (2/(2 + qAbs(u1)*hx) + (u1 + qAbs(u1))*hx*0.5)/(hx*hx);
                A2 = (2/(2 + qAbs(v1)*hy) + (v1 + qAbs(v1))*hy*0.5)/(hy*hy);
                B1 = (2/(2 + qAbs(u2)*hx) - (u2 - qAbs(u2))*hx*0.5)/(hx*hx);
                B2 = (2/(2 + qAbs(v2)*hy) - (v2 - qAbs(v2))*hy*0.5)/(hy*hy);
                C = A1 + A2 + B1 + B2;

                omegaNew = (1 - wOmega)*omega[i][j] + wOmega*(A1*omega[i][j - 1] + A2*omega[i - 1][j] +
                                                              B1*omega[i][j + 1] + B2*omega[i + 1][j] +
                                                              Gr*(t[i][j + 1] - t[i][j - 1])/(2*hx))/C;
                normEpsOmega += (omegaNew - omega[i][j])*(omegaNew - omega[i][j]);
                omega[i][j] = omegaNew;
                //--------------------------------------------------------------------------------------------
                //--------------------------------------------------------------------------------------------
                //находим новое приближение решения для функции тока
                psiNew = (1 - wPsi)*psi[i][j] + wPsi*((psi[i][j - 1] + psi[i][j + 1])/(hx*hx) +
                                                      (psi[i - 1][j] + psi[i + 1][j])/(hy*hy) +
                                                      omega[i][j])/(2./(hx*hx) + 2./(hy*hy));
                normEpsPsi += (psiNew - psi[i][j])*(psiNew - psi[i][j]);
                psi[i][j] = psiNew;
            }

            //краевые условия справа
            t[i][nx - 1] = 1.0 - i/(ny - 1.0);
            psi[i][nx - 1] = 0.0;

            if(isRightWallHard)
                omega[i][nx - 1] = (1 - wOmega)*omega[i][nx - 1] +
                                   wOmega*(-0.5*omega[i][nx - 2] - 3.0*psi[i][nx - 2]/(hx*hx));
            else
                omega[i][nx - 1] = 0;
        }

        //краевые условия на верхней границе
        for(int j = 0; j < nx; j++)
        {
            t[ny - 1][j] = 0;
            psi[ny - 1][j] = 0.0;

            if(isTopWallHard)
                omega[ny - 1][j] = (1 - wOmega)*omega[ny - 1][j] +
                                   wOmega*(-0.5*omega[ny - 2][j] - 3.0*psi[ny - 2][j]/(hy*hy));
            else
                omega[ny - 1][j] = 0;
        }

        //краевые условия в угловых точках
        omega[0][nx - 1] = 0.0;
        omega[ny - 1][nx - 1] = 0.0;

        emit iterationFinished(currentIter,maxNumOfIter,residual(normEpsT, normEpsPsi, normEpsOmega));

        mutex.lock();
    }while(++currentIter < maxNumOfIter && !isSolutionCutOff && residual(normEpsT,normEpsPsi,normEpsOmega) > 1e-6);

    formV();

    if(currentIter >= maxNumOfIter && !isSolutionCutOff)
        emit maxIterNumberAttained(residual(normEpsT, normEpsPsi, normEpsOmega));
    if(!isSolutionCutOff)
    {
        output();
        emit solutionFinished();
    }
    mutex.unlock();

    freeMemory();
}

//============================================================================================================
//Функция выделяет динамическую память для данных, создает конечноразностную сетку,
//а также задает начальное приближение для искомых функций T, Psi и Omega.
//============================================================================================================
void BottomHeatingSolver::prepareData()
{
    //********************************************************************************************************
    //Создание сетки.
    //********************************************************************************************************
    hx = width/(nx - 1);
    hy = height/(ny - 1);

    x = new double[nx];
    y = new double[ny];
    for(int i = 0; i < nx; i++)
        x[i] = i*hx;
    for(int j = 0; j < ny; j++)
        y[j] = j*hy;

    //********************************************************************************************************
    //Выделение памяти под матрицы неизвестных. Задание начальных приближений матриц неизвестных.
    //********************************************************************************************************
    t = new double*[ny];
    for(int i = 0; i < ny; i++)
    {
        t[i] = new double[nx];
        for(int j = 0; j < nx; j++)
            t[i][j] = QRandomGenerator64::global()->generateDouble();
    }

    psi = new double*[ny];
    for(int i = 0; i < ny; i++)
    {
        psi[i] = new double[nx];
        for(int j = 0; j < nx; j++)
            psi[i][j] = 0;
    }

    omega = new double*[ny];
    for(int i = 0; i < ny; i++)
    {
        omega[i] = new double[nx];
        for(int j = 0; j < nx; j++)
            omega[i][j] = 0;
    }

    vx = new double*[ny];
    for(int i = 0; i < ny; i++)
        vx[i] = new double[nx];

    vy = new double*[ny];
    for(int i = 0; i < ny; i++)
        vy[i] = new double[nx];
}

//============================================================================================================
//Освобождение динамически выделенной памяти.
//============================================================================================================
void BottomHeatingSolver::freeMemory()
{
    for(int i = 0; i < ny; i++)
        delete [] t[i];
    delete [] t;
    t = NULL;

    for(int i = 0; i < ny; i++)
        delete [] psi[i];
    delete [] psi;
    psi = NULL;

    for(int i = 0; i < ny; i++)
        delete [] omega[i];
    delete [] omega;
    omega = NULL;

    for(int i = 0; i < ny; i++)
        delete [] vx[i];
    delete [] vx;
    vx = NULL;

    for(int i = 0; i < ny; i++)
        delete [] vy[i];
    delete [] vy;
    vy = NULL;

    delete [] x;
    delete [] y;
    x = NULL;
    y = NULL;
}

//============================================================================================================
//Вывод результатов.
//============================================================================================================
void BottomHeatingSolver::output()
{
    QFile file;
    QTextStream ostream(&file);
    ostream.setRealNumberPrecision(13);

    //********************************************************************************************************
    //выводим информацию о сетке.
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/net.dat");
    file.open(QIODevice::WriteOnly);
    ostream << nx << Qt::endl;
    ostream << ny << Qt::endl;
    ostream << hx << Qt::endl;
    ostream << hy << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения функции температуры
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/T.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < ny; i++)
        for(int j = 0; j < nx; j++)
            ostream << x[j] << " " << y[i] << " " << t[i][j] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения функции тока
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/Psi.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < ny; i++)
        for(int j = 0; j < nx; j++)
            ostream << x[j] << " " << y[i] << " " << psi[i][j] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения вихря
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/Omega.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < ny; i++)
        for(int j = 0; j < nx; j++)
            ostream << x[j] << " " << y[i] << " " << omega[i][j] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения x-составляющей функции скорости
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/Vx.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < ny; i++)
        for(int j = 0; j < nx; j++)
            ostream << x[j] << " " << y[i] << " " << vx[i][j] << Qt::endl;
    file.close();

    //********************************************************************************************************
    //выводим значения y-составляющей функции скорости
    //********************************************************************************************************
    file.setFileName(qApp->applicationDirPath() + "/result/Vy.dat");
    file.open(QIODevice::WriteOnly);
    for(int i = 0; i < ny; i++)
        for(int j = 0; j < nx; j++)
            ostream << x[j] << " " << y[i] << " " << vy[i][j] << Qt::endl;
    file.close();
}

//============================================================================================================
//Расчет невязки по нелинейности с учетом коэффициента релаксации.
//============================================================================================================
double BottomHeatingSolver::residual(double normEpsT, double normEpsPsi, double normEpsOmega)
{
    double normT(0);
    double normPsi(0);
    double normOmega(0);

    for(int i=0; i < ny; i++)
        for(int j = 0; j < nx; j++)
        {
            normT += t[i][j]*t[i][j];
            normPsi += psi[i][j]*psi[i][j];
            normOmega += omega[i][j]*omega[i][j];
        }

    return qMax(sqrt(normEpsT/normT)/wT,qMax(sqrt(normEpsPsi/normPsi)/wPsi,sqrt(normEpsOmega/normOmega)/wOmega));
}

//============================================================================================================
//Вычисление вектора скорости.
//============================================================================================================
void BottomHeatingSolver::formV()
{
    //считаем x-компоненту скорсти
    for(int i = 1; i < ny - 1; i++)
        for(int j = 0; j < nx; j++)
            vx[i][j] = (psi[i + 1][j] - psi[i][j])/hy;

    //считаем y-компоненту скорости
    for(int i = 0; i < ny; i++)
        for(int j = 1; j < nx - 1; j++)
            vy[i][j] = -(psi[i][j + 1] - psi[i][j])/hx;

    if(isBottomWallHard)
        for(int j = 0; j < nx; j++)
            vx[0][j] = 0;
    else
        for(int j = 0; j < nx; j++)
            vx[0][j] = vx[1][j];

    if(isTopWallHard)
        for(int j = 0; j < nx; j++)
            vx[ny - 1][j] = 0;
    else
        for(int j = 0; j < nx; j++)
            vx[ny - 1][j] = vx[ny - 2][j];

    if(isLeftWallHard)
        for(int i = 0; i < ny; i++)
            vy[i][0] = 0;
    else
        for(int i = 0; i < ny; i++)
            vy[i][0] = vy[i][1];

    if(isRightWallHard)
        for(int i = 0; i < ny; i++)
            vy[i][nx - 1] = 0;
    else
        for(int i = 0; i < ny; i++)
            vy[i][nx - 1] = vy[i][nx - 2];
}

//============================================================================================================
//============================================================================================================
double BottomHeatingSolver::getDefaultWT(){return 0.8;}
double BottomHeatingSolver::getDefaultWPsi(){return 1.1;}
double BottomHeatingSolver::getDefaultWOmega(){return 1.0;}
int BottomHeatingSolver::getDefaultMaxNumOfIter(){return 5000;}

//============================================================================================================
//============================================================================================================
BottomHeatingSolver::~BottomHeatingSolver()
{
    if(t != NULL)
    {
        for(int i = 0; i < ny; i++)
            delete [] t[i];
        delete [] t;
    }

    if(psi != NULL)
    {
        for(int i = 0; i < ny; i++)
            delete [] psi[i];
        delete [] psi;
    }

    if(omega != NULL)
    {
        for(int i = 0; i < ny; i++)
            delete [] omega[i];
        delete [] omega;
    }

    if(vx != NULL)
    {
        for(int i = 0; i < ny; i++)
            delete [] vx[i];
        delete [] vx;
    }

    if(vy != NULL)
    {
        for(int i = 0; i < ny; i++)
            delete [] vy[i];
        delete [] vy;
    }

    if(x != NULL)
        delete [] x;

    if(y != NULL)
        delete [] y;
}
