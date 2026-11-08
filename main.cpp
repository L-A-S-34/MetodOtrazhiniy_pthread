#include "SolvePthreads.h"
#include "init.h"


#include<iostream>
#include<fstream>
#include<algorithm>
#include<iomanip>
#include<cmath>
#include<string>
#include<chrono>

int main(int argc, char* argv[])
{
	int k, n, m; // формула матрицы, размерность матрицы, кол-во выводимых значений
	int num_threads = 1;

	if (argc < 4)
	{
		std::cout << "There are not enough arguments" << '\n';
		return 1;
	}

	try
	{
		n = std::stoi(argv[1]);
		m = std::stoi(argv[2]);
		k = std::stoi(argv[3]);
	}
	catch (...)
	{
		std::cout << "Invalid parameter format" << '\n';
		return 1;
	}

	if ((argc == 6) && (k == 0))
	{
		try
		{
			num_threads = std::stoi(argv[5]);
		}
		catch (...)
		{
			std::cout << "Invalid parameter format" << '\n';
			return 1;
		}
	}
	if ((argc == 5) && (k != 0))
	{
		try
		{
			num_threads = std::stoi(argv[4]);
		}
		catch (...)
		{
			std::cout << "Invalid parameter format" << '\n';
			return 1;
		}
	}

	if ((n <= 0) || (m <= 0) || (k < 0) || (k > 4))
	{
		std::cout << "Invalid parameter format" << '\n';
		return 1;
	}

	std::string filename = "";

	if (k == 0)
	{
		if (argc < 5)
		{
			std::cout << "There are not enough arguments" << '\n';
			return 1;
		}
		filename = argv[4];
	}
	else
	{
		if (argc >= 6)
		{
			std::cout << "An excess of arguments" << '\n';
			return 1;
		}
	}

	std::cout << "Size of double: " << sizeof(double) << " bytes" << std::endl;
	std::cout << "Size of long double: " << sizeof(long double) << " bytes" << std::endl;

	std::cout << '\n' << "type: long double" << '\n' << std::endl;

	std::cout << '\n' << "type: double + parallel" << '\n' << std::endl;

	{
		double* A = new double[n * n]; // Матрица
		double* b = new double[n]; // Правая часть
		double* x = new double[n]; // Решение

		auto start_time = std::chrono::high_resolution_clock::now();
		int q = Init<double>(A, b, k, n, m, filename);
		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double>  duration = end_time - start_time;
		std::cout << std::defaultfloat;
		std::cout << "(Kahan+LD)System initialization time: " << (duration.count()) * 100.0 << " hundredths of a second" << '\n';

		if (q == 0)
		{
			std::cout << "Matrix: " << '\n';
			printMatrix<double>(A, n, n, m);


			start_time = std::chrono::high_resolution_clock::now();
			q = SolveParallel<double>(A, n, b, x, num_threads);
			end_time = std::chrono::high_resolution_clock::now();


			if (q != 1)
			{
				duration = end_time - start_time;
				std::cout << std::defaultfloat;
				std::cout << "(Kahan+LD +Par)System solution time: " << (duration.count()) * 100.0 << " hundredths of a second" << '\n';

				std::cout << "(Kahan+LD+Par)Solution: " << '\n';
				printMatrix<double>(x, n, 1, m);

				q = Init<double>(A, b, k, n, m, filename);
				if (q == 0)
				{
					std::cout << "(Kahan+LD+Par)Norm of residual: " << std::scientific << std::setprecision(3) << CalcNormNevParallel<double>(A, b, x, n, num_threads) << '\n';

					std::cout << "(Kahan+LD+Par)Solution error margin: " << CalcErNormParallel<double>(x, n, num_threads) << '\n';
				}
			}
		}

		delete[] A;
		delete[] b;
		delete[] x;

	}

	std::cout << '\n' << "type: long double + parallel" << '\n' << std::endl;

	{
		long double* A = new long double[n * n]; // Матрица
		long double* b = new long double[n]; // Правая часть
		long double* x = new long double[n]; // Решение

		auto start_time = std::chrono::high_resolution_clock::now();
		int q = Init<long double>(A, b, k, n, m, filename);
		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double>  duration = end_time - start_time;
		std::cout << std::defaultfloat;
		std::cout << "(Kahan+LD)System initialization time: " << (duration.count()) * 100.0 << " hundredths of a second" << '\n';

		if (q == 0)
		{
			std::cout << "Matrix: " << '\n';
			printMatrix<long double>(A, n, n, m);


			start_time = std::chrono::high_resolution_clock::now();
			q = SolveParallel<long double>(A, n, b, x, num_threads);
			end_time = std::chrono::high_resolution_clock::now();


			if (q != 1)
			{
				duration = end_time - start_time;
				std::cout << std::defaultfloat;
				std::cout << "(Kahan+LD +Par)System solution time: " << (duration.count()) * 100.0 << " hundredths of a second" << '\n';

				std::cout << "(Kahan+LD+Par)Solution: " << '\n';
				printMatrix<long double>(x, n, 1, m);

				q = Init<long double>(A, b, k, n, m, filename);
				if (q == 0)
				{
					std::cout << "(Kahan+LD+Par)Norm of residual: " << std::scientific << std::setprecision(3) << CalcNormNevParallel<long double>(A, b, x, n, num_threads) << '\n';

					std::cout << "(Kahan+LD+Par)Solution error margin: " << CalcErNormParallel<long double>(x, n, num_threads) << '\n';
				}
			}
		}

		delete[] A;
		delete[] b;
		delete[] x;

	}

	return 0;
}