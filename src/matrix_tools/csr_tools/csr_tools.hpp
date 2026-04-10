#pragma once

namespace csrtools
{

    /*! \fn void partFactorLU(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu,
     *                        double *l, double *di_f, double *u)
     *
     * \brief Неполная LU факторизация матрицы.
     *
     * \param dimension     Размерность матрицы.
     * \param [in]  ig      Информация о количестве ненулевых элементов в каждой строке матрицы.
     * \param [in]  jg      Информация о номерах столбцов ненулевых элементов матрицы.
     * \param [in]  ggl     Нижний треугольник исходной матрицы.
     * \param [in]  di      Диагональ исходной матрицы.
     * \param [in]  ggu     Верхний треугольник исходной матрицы.
     * \param [out] l       Нижний треугольник матрицы факторизации.
     * \param [out] di_f    Диагональ матрицы факторизации.
     * \param [out] u       Верхний треугольник матрицы факторизации.
     */
    void partFactorLU(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu,
                      double *l, double *di_f, double *u);

    /*! \fn void matrixMultByVector(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu,
     *                              double *vector, double *result)
     *
     * \brief Умножение матрицы на вектор.
     *
     * \param dimension     Размерность матрицы и вектора.
     * \param [in]  ig      Информация о количестве ненулевых элементов в каждой строке матрицы.
     * \param [in]  jg      Информация о номерах столбцов ненулевых элементов матрицы.
     * \param [in]  ggl     Нижний треугольник матрицы.
     * \param [in]  di      Диагональ матрицы.
     * \param [in]  ggu     Верхний треугольник матрицы.
     * \param [in]  vector  Вектор, на который умножается матрица.
     * \param [out] result  Результат произведения.
     */
    void matrixMultByVector(int dimension, int *ig, int *jg, double *ggl, double *di, double *ggu,
                            double *vector, double *result);

    /*! \fn int position(int i, int j)
     *
     *  \brief Индекс элемента матрицы A[i,j] в массиве ggl.
     *
     *  \param [in]  ig     Информация о количестве ненулевых элементов в каждой строке матрицы.
     *  \param [in]  jg     Информация о номерах столбцов ненулевых элементов матрицы.
     *  \param i			Номер строки матрицы, которой принадлежит элемент A[i,j].
     *  \param j			Номер столбца матрицы, которому принадлежит элемент A[i,j]
     *  \return Индекс элемента в массиве ggl или -1 если элемент не найден.
     */
    int position(int *ig, int *jg, int i, int j);
}
