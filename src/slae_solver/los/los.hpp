#pragma once

/*! \class LOS
 *  \brief Класс, реализующий решение СЛАУ методом ЛОС.
 */
class LOS
{
public:

    /*! \fn static void solve(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu, double *f,
     *                        double *x, int maxNumOfIter = 1000, double sufficientResidual = 1e-6)
     *
     * \brief Решение СЛАУ с матрицей в разреженном строчном формате.
     *
     * \param dimension          Размерность матрицы.
     * \param [in]      ig       Информация о количестве ненулевых элементов в каждой строке матрицы.
     * \param [in]      jg       Информация о номерах столбцов ненулевых элементов матрицы.
     * \param [in]      ggl      Нижний треугольник матрицы.
     * \param [in]      di       Диагональ матрицы.
     * \param [in]      ggu      Верхний треугольник матрицы.
     * \param [in]      f        Правая часть СЛАУ.
     * \param [in,out]	x        На входе - начальное приближение решения СЛАУ. На выходе - решение СЛАУ.
     * \param maxNumOfIter       Максимальное число итераций решателя. После того, как решатель совершит
     *                           данное количество итераций, его работа прекращается в независимости от
     *                           того, найдено ли решение.
     * \param sufficientResidual Значение невязки при котором решение СЛАУ считается найденым.
     */
    static void solve(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu, double *f,
                      double *x, int maxNumOfIter = 1000, double sufficientResidual = 1e-6);
};
