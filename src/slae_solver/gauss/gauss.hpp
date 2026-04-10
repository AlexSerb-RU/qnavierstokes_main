#pragma once

/*! \class Gauss
 * \brief Класс, реализующий решение СЛАУ методом Гаусса.
 */
class Gauss
{
public:
    /*! \fn static void solveLowSLAE(int dimension, int *ig, int *jg, double *ggl, double *di, double *f)
     *
     * \brief Решение СЛАУ с нижнетреугольной матрицей в разреженном строчном формате.
     *
     * \param dimension         Размерность матрицы.
     * \param [in]      ig      Информация о количестве ненулевых элементов в каждой строке матрицы.
     * \param [in]      jg      Информация о номерах столбцов ненулевых элементов матрицы.
     * \param [in]      ggl     Нижний треугольник матрицы.
     * \param [in]      di      Диагональ матрицы.
     * \param [in,out]  f       На входе - правая часть СЛАУ. На выходе - решение СЛАУ.
     */
    static void solveLowSLAE(int dimension, int *ig, int *jg, double *ggl, double *di, double *f);

    /*! \fn static void solveUpSLAE(int dimension, int *ig, int *jg, double *ggu, double *f)
     *
     * \brief Решение СЛАУ с верхнетреугольной матрицей в разреженном строчном формате (на диагонали единицы).
     *
     * \param dimension         Размерность матрицы.
     * \param [in]      ig      Информация о количестве ненулевых элементов в каждой строке матрицы.
     * \param [in]      jg      Информация о номерах столбцов ненулевых элементов матрицы.
     * \param [in]      ggu     Верхний треугольник матрицы.
     * \param [in,out]  f       На входе - правая часть СЛАУ. На выходе - решение СЛАУ.
     */
    static void solveUpSLAE(int dimension, int *ig, int *jg, double *ggu, double *f);
};
