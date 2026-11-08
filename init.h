#pragma once
#include<iostream>  
#include<fstream>
#include<algorithm>
#include<cmath>
#include<iomanip>
#include<string>

template <typename T>
T f(const int k, const int n, const int i, const int j)
{
	if ((k <= 0) || (k > 4) || (n <= 0))
	{
		std::cout << "The parameters k, n, m are entered incorrectly." << '\n';
	}

	if (k == 1)
	{
		return T(n) - T(std::max(T(i + 1), T(j + 1)) + 1);
	}

	if (k == 2)
	{
		return T(std::max(T(i + 1), T(j + 1)));
	}

	if (k == 3)
	{
		return T(std::abs(T(i - j)));
	}

	if (k == 4)
	{
		return T(1.0) / (T(i + j + 1));
	}
	return T(0);
}

template <typename T>
void printMatrix(const T* A, const int rows, const int cols, const int m)  //вывод матрицы
{
	int drows = std::min(rows, m);
	int dcols = std::min(cols, m);

	std::cout << std::scientific;

	for (int i = 0; i < drows; i++)
	{
		for (int j = 0; j < dcols; j++)
		{
			std::cout << " ";
			std::cout << std::setw(10) << std::setprecision(3) << A[i * cols + j];
		}
		std::cout << '\n';
	}

	std::cout << std::defaultfloat;
}

template <typename T>
int Init(T* A, T* b, const int k, const int n, const int m, std::string filename) // ¬вод матрицы
{
	if ((k < 0) || (k > 4) || (n <= 0) || (m <= 0))
	{
		std::cout << "The parameters k, n, m are entered incorrectly." << '\n';
	}

	if (k == 0)
	{
		std::ifstream File(filename);

		if (!File.is_open())
		{
			std::cout << "File opening error" << '\n';
			return 1;
		}

		T number;
		int i = 0;

		while (i < n * n)
		{
			if (File >> number)
			{
				A[i] = number;
				i += 1;
			}
			else
			{
				if (File.eof())
				{
					std::cout << "There is not enough data for the matrix." << '\n';
				}
				else
				{
					std::cout << "Invalid data format" << '\n';
				}
				return 1;
			}
		}
	}
	else
	{
		int i, j;

		for (i = 0; i < n; i++)
		{
			for (j = 0; j < n; j++)
			{
				A[i * n + j] = f<T>(k, n, i, j);
			}
		}
	}

	int s, l;

	for (s = 0; s < n; s++) // реализаци€ столбца b
	{
		b[s] = 0;
		for (l = 0; 2 * l < n; l++)
		{
			b[s] += A[s * n + 2 * l];
		}
	}

	return 0;
} 