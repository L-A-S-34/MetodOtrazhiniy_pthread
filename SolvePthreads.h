#pragma once

#include<cmath>
#include<iostream>  

#define NOMINMAX //проблемы с std::min и std::max на виндовс
#ifdef _WIN32
#include <windows.h>
typedef HANDLE pthread_t; //Handle - 8 байт, объявляем pthread как handle, просто смена имени
inline int pthread_create(pthread_t* thread, void* attr, void* (*Worker)(void*), void* arg)
{
	Worker(arg);
	return 0;
}
inline int pthread_join(pthread_t, void**) 
{
	return 0;
}
#else
#include <pthread.h>
#endif

template <typename T>
void KahanStep(T Add, T& sum, T& err) //шаг алгоритма Кэхэна
{
	T y = Add - err;
	T t = sum + y;
	err = (t - sum) - y;
	sum = t;
}

template<typename T>
struct ThreadDataSolve //структура для хранения данных для Solve
{
	T* A; //матрица
	int n; // размер матрицы
	int k; // номер шага
	T* v; // v
	T a_coef; // 2 / норма v ^2

	int start_col; //начало и конец столбца для расчета;
	int end_col;
};

template<typename T>
struct ThreadDataNormNev //структура для хранения данных для подсчета нормы невязки
{
	const T* A; // матрица
	const T* x; // решение
	const T* b; // правая часть
	int n; // размер матрицы

	int start_row; //начало и конец строки для расчета; конец - не включительно
	int end_row;

	T local_norm_r;
	T local_norm_b;
	T local_err_r;  // ошибка для local_norm_r
	T local_err_b;  // ошибка для local_norm_b
};

template<typename T>
struct ThreadDataNormErr //структура для хранения данных для подсчета нормы ршибки 
{
	const T* x; // решение
	int n; // размер матрицы

	int start_row; //начало и конец строки для расчета; конец - не включительно
	int end_row;

	T local_sum;
	T local_err; //ошибка для local_sum
};



template<typename T>
void* normErrWorker(void* arg) // воркер для нормы ошибки
{
	ThreadDataNormErr<T>* data = static_cast<ThreadDataNormErr<T>*>(arg);

	data->local_sum = T(0);
	data->local_err = T(0);

	for (int i = data->start_row; i < data->end_row; i++) {
		T exact_val = (i % 2 == 0) ? T(1) : T(0);
		T diff = data->x[i] - exact_val;
		KahanStep(diff * diff, data->local_sum, data->local_err);
	}

	return nullptr;
}

template <typename T>
void* normNevWorker(void* arg) //воркер для нормы невязки
{
	ThreadDataNormNev<T>* data = static_cast<ThreadDataNormNev<T>*>(arg);

	data->local_norm_r = T(0);
	data->local_norm_b = T(0);
	data->local_err_r = T(0);
	data->local_err_b = T(0);

	int n = data->n;

	for (int i = data->start_row; i < data->end_row; i++) {
		T Ax_i = T(0); // i-я строка Ax
		T err_ax = T(0); //ошибка для подсчета Ax

		for (int j = 0; j < n; j++) {
			KahanStep(data->A[i * n + j] * data->x[j], Ax_i, err_ax);
		}
		
		KahanStep(-data->b[i], Ax_i, err_ax);

		KahanStep(Ax_i * Ax_i, data->local_norm_r, data->local_err_r);
		KahanStep(data->b[i] * data->b[i], data->local_norm_b, data->local_err_b);
	}

	return nullptr;
}

template <typename T>
void* SolveWorker(void* arg) //воркер для нормы невязки
{
	ThreadDataSolve<T>* data = static_cast<ThreadDataSolve<T>*>(arg);

	int n = data->n;
	int k = data->k;
	T* A = data->A;

	int start_col = data->start_col;
	int end_col = data->end_col;
	T* v = data->v;
	T a = data->a_coef; // A -> A - a * v~ * v~^t * A
	T product_A;
	T err = T(0);

	for (int j = start_col; j < end_col; j++) // делаем по столбцам j = k+1,n
	{
		product_A = v[0] * A[k * n + j]; // считаем v~^t * A

		err = T(0);
		for (int i = k + 1; i < n; i++)
		{
			KahanStep(v[i - k] * A[i * n + j], product_A, err);
		}

		A[k * n + j] -= v[0] * a * product_A; // A - ...
		for (int i = k + 1; i < n; i++)
		{
			A[i * n + j] -= a * v[i - k] * product_A;
		}
	}
	return nullptr;
}

template<typename T>
T CalcErNormParallel(const T* x, const int n, int num_threads) //координирующая функция для нормы ошибки
{ 
	pthread_t* threads = new pthread_t[num_threads];
	ThreadDataNormErr<T>* thread_data = new ThreadDataNormErr<T>[num_threads];


	int rows_per_thread = n / num_threads; // кол-во строк на поток
	int remainder = n % num_threads; // остаток строк
	int start = 0;

	for (int t = 0; t < num_threads; t++) //заполняем потоки
	{
		thread_data[t].x = x; //указатель передаем
		thread_data[t].n = n;
		thread_data[t].start_row = start;

		int extra = (t < remainder) ? 1 : 0; // первые несколько потоков получат на 1 строку больше
		thread_data[t].end_row = start + rows_per_thread + extra;
		start = thread_data[t].end_row;

		pthread_create(&threads[t], nullptr, normErrWorker<T>, &thread_data[t]); // создание и запуск потока
	}

	T total_sum = T(0); 
	T total_err = T(0);
	for (int t = 0; t < num_threads; t++) // синхронизация
	{
		pthread_join(threads[t], nullptr);
		
		total_err += thread_data[t].local_err; // учитываем локальную ошибку
		KahanStep(thread_data[t].local_sum, total_sum, total_err);
	}

	delete[] threads;
	delete[] thread_data;

	return std::sqrt(total_sum);
}

template<typename T>
T CalcNormNevParallel(const T* A, const T* b, const T* x, const int n, int num_threads) //координирующая функция для нормы невязки
{
	pthread_t* threads = new pthread_t[num_threads];
	ThreadDataNormNev<T>* thread_data = new ThreadDataNormNev<T>[num_threads];

	int rows_per_thread = n / num_threads; // кол-во строк на поток
	int remainder = n % num_threads; // остаток строк
	int start = 0;

	for (int t = 0; t < num_threads; t++) //заполняем потоки
	{
		thread_data[t].A = A;
		thread_data[t].b = b;
		thread_data[t].x = x;
		thread_data[t].n = n;
		thread_data[t].start_row = start;

		int extra = (t < remainder) ? 1 : 0;
		thread_data[t].end_row = start + rows_per_thread + extra;
		start = thread_data[t].end_row;

		pthread_create(&threads[t], nullptr, normNevWorker<T>, &thread_data[t]);
	}

	T total_norm_r = T(0);
	T total_norm_b = T(0);
	T final_err_r = T(0);
	T final_err_b = T(0);

	for (int t = 0; t < num_threads; t++) {
		pthread_join(threads[t], nullptr);

		// сохраняем ошибки, может быть небольшая разница со стандартным алгоритмом
		final_err_r += thread_data[t].local_err_r;
		final_err_b += thread_data[t].local_err_b;

		// суммируем
		KahanStep(thread_data[t].local_norm_r, total_norm_r, final_err_r);
		KahanStep(thread_data[t].local_norm_b, total_norm_b, final_err_b);
	}

	delete[] threads;
	delete[] thread_data;

	total_norm_r = std::sqrt(total_norm_r);
	total_norm_b = std::sqrt(total_norm_b);

	if (total_norm_b > T(0)) {
		return total_norm_r / total_norm_b;
	}
	return total_norm_r;
}


template <typename T>
int SolveParallel(T* A, const int n, T* b, T* x, int num_threads) //решение системы с использованием Kahan
{
	pthread_t* threads = new pthread_t[num_threads];
	ThreadDataSolve<T>* thread_data = new ThreadDataSolve<T>[num_threads];

	T err = T(0); //корреция ошибки
	for (int k = 0; k < n - 1; k++) // приводим к треугольному виду
	{
		err = T(0);
		T norm_x = T(0); // норма k - го столбца

		for (int i = k; i < n; i++)
		{
			KahanStep(A[i * n + k] * A[i * n + k], norm_x, err);
		}
		err = T(0);

		norm_x = std::sqrt(norm_x);

		if (norm_x > T(0))
		{

			T sign = (A[k * n + k] >= T(0)) ? T(1) : T(-1); //ищем вектор v и его норму

			T* v = new T[n - k];
			v[0] = A[k * n + k] + sign * norm_x;

			T norm_v_2 = v[0] * v[0];
			for (int i = k + 1; i < n; i++)
			{
				v[i - k] = A[i * n + k];
				KahanStep(A[i * n + k] * A[i * n + k], norm_v_2, err);
			}
			err = 0;

			if (norm_v_2 > T(0)) // если норма v !=0 применяем отражение
			{
				T a = (T(2)) / norm_v_2; // A -> A - a * v~ * v~^t * A
				T product_b;

				int total_cols = n - (k + 1); //правая граница не включается
				int cols_per_thread = total_cols / num_threads;
				int remainder = total_cols % num_threads;
				int start = k + 1;


				for (int t = 0; t < num_threads; t++)
				{
					thread_data[t].A = A;
					thread_data[t].n = n;
					thread_data[t].k = k;
					thread_data[t].v = v;
					thread_data[t].a_coef = a;
					thread_data[t].start_col = start;

					int extra = (t < remainder) ? 1 : 0;
					thread_data[t].end_col = start + cols_per_thread + extra;
					start = thread_data[t].end_col;

					pthread_create(&threads[t], nullptr, SolveWorker<T>, &thread_data[t]);
				}

				product_b = v[0] * b[k]; // считаем v~^t * b
				for (int i = k + 1; i < n; i++)
				{
					KahanStep(v[i-k] * b[i], product_b, err);
				}
				err = T(0);

				b[k] -= a * v[0] * product_b; // b - ...
				for (int i = k + 1; i < n; i++)
				{
					b[i] -= a * v[i-k] * product_b;
				}

				for (int t = 0; t < num_threads; t++)
				{
					pthread_join(threads[t], nullptr); 
				}

				A[k * n + k] = -sign * norm_x; // мы всегда знаем каким будет "первый" столбец
				for (int i = k + 1; i < n; i++)
				{
					A[i * n + k] = T(0);
				}
			}
			delete[] v;
		}
	}
	delete[] threads;
	delete[] thread_data;

	T sum = T(0);
	err = T(0);
	for (int i = n - 1; i >= 0; i--) // решение треугольной системы
	{
		sum = T(0);
		for (int j = i + 1; j < n; j++)
		{
			KahanStep(A[i * n + j] * x[j], sum, err);
		}
		err = T(0);

		if ((A[i * n + i] > 0) || (A[i * n + i] < 0))
		{
			x[i] = (b[i] - sum) / A[i * n + i]; //здесь слишком большая погрешность, не спасает; даже Kahan
		}
		else
		{
			std::cout << "The system is degenerate; there is no unique solution." << '\n';
			return 1;
		}
	}
	return 0;
}