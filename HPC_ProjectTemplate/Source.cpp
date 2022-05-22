#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#include<mpi.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* Properties_Image(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* Output_Image;


	int Read_Image_Width, Read_Image_Height;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap Image_Array(imagePath);

	Read_Image_Width = Image_Array.Width;
	Read_Image_Height = Image_Array.Height;
	*w = Image_Array.Width;
	*h = Image_Array.Height;
	int* Red_Channel = new int[Image_Array.Height * Image_Array.Width];
	int* Green_Channel = new int[Image_Array.Height * Image_Array.Width];
	int* Blue_Channel = new int[Image_Array.Height * Image_Array.Width];
	Output_Image = new int[Image_Array.Height * Image_Array.Width];
	for (int i = 0; i < Image_Array.Height; i++)
	{
		for (int j = 0; j < Image_Array.Width; j++)
		{
			System::Drawing::Color Color_Details = Image_Array.GetPixel(j, i);

			Red_Channel[i * Image_Array.Width + j] = Color_Details.R;
			Blue_Channel[i * Image_Array.Width + j] = Color_Details.B;
			Green_Channel[i * Image_Array.Width + j] = Color_Details.G;

			Output_Image[i * Image_Array.Width + j] = ((Color_Details.R + Color_Details.B + Color_Details.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return Output_Image;
}


void Draw_New_Image(int* image, int width, int height)
{
	System::Drawing::Bitmap New_Image_Array(width, height);


	for (int i = 0; i < New_Image_Array.Height; i++)
	{
		for (int j = 0; j < New_Image_Array.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color Color_Details = System::Drawing::Color::FromArgb(image[i * New_Image_Array.Width + j], image[i * New_Image_Array.Width + j], image[i * New_Image_Array.Width + j]);
			New_Image_Array.SetPixel(j, i, Color_Details);
		}
	}


	New_Image_Array.Save("..//Data//Output//outputRes.png");
	cout << "result Image Saved " << endl;
}

float** Calculate_Kernel(int Kernel_Size) {
	float** Kernel_Filter = new float* [Kernel_Size];
	for (int i = 0; i < Kernel_Size; i++)
	{
		Kernel_Filter[i] = new float[Kernel_Size];
		for (int j = 0; j < Kernel_Size; j++)
		{
			Kernel_Filter[i][j] = 1.0 / (Kernel_Size * Kernel_Size * 1.0);
		}
	}
	return Kernel_Filter;
}


int main()
{
	int Current_Image_Width = 4, Current_Image_Height = 4;

	int Start_Time, End_Time, Full_Time = 0;

	System::String^ Image_Path;
	std::string Current_Image;
	Current_Image = "..//Data//Input//N.png";

	Image_Path = marshal_as<System::String^>(Current_Image);
	int* Image_Array = Properties_Image(&Current_Image_Width, &Current_Image_Height, Image_Path); //pixel values of image

	

	///start code here

	MPI_Init(NULL, NULL);///////////////////// mpiexec "HPC_ProjectTemplate.exe"
	int Processors_Ranks;
	MPI_Comm_rank(MPI_COMM_WORLD, &Processors_Ranks);
	int Processors_Size;
	MPI_Comm_size(MPI_COMM_WORLD, &Processors_Size);
	int Lower_Rank = 0;
	int Final_Rank = Processors_Size - 1;
	float** Kernel_Filter;
	int Kernel_Size;
	if (Processors_Ranks == Lower_Rank) {

		cout << "Please Enter The Kernel Size (ODD Number): ";
		cin >> Kernel_Size;
		if (Kernel_Size % 2 == 0)
		{
			Kernel_Size -= 1;
		}
	}
	MPI_Bcast(&Kernel_Size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	Kernel_Filter = Calculate_Kernel(Kernel_Size);
	
	Start_Time = clock();
	
	//start and end rows
	int Divided_Image_Rows = Current_Image_Height / Processors_Size;
	int Start_Image_Rows = Divided_Image_Rows * Processors_Ranks;
	int End_Image_Rows = Start_Image_Rows + Divided_Image_Rows;
	
	if (Processors_Ranks == Final_Rank)
		End_Image_Rows = Current_Image_Height;

	cout << Processors_Ranks << ' ' << Start_Image_Rows << ' ' << End_Image_Rows << ' ' << endl;


	int Work_Rows = End_Image_Rows - Start_Image_Rows;
	int Work_Size = (Work_Rows)*Current_Image_Width;
	int* Work_Image_Array = new int[Work_Size];
	//kernel compution
	for (int i = Start_Image_Rows; i < End_Image_Rows; i++)//image rows
	{
		for (int j = 0; j < Current_Image_Width; j++)//image columns
		{
			//(i,j) * kernel
			int New_Value = 0;
			int X_Axis = -(Kernel_Size / 2);
			int Y_Axis = -(Kernel_Size / 2);
			for (int m = 0; m < Kernel_Size; m++)//kernel rows
			{
				for (int l = 0; l < Kernel_Size; l++)//kernel columns
				{

					if ((j + Y_Axis >= Current_Image_Width) || (j + Y_Axis < 0)) {//columns
						Y_Axis++;
						continue;
					}
					if ((Processors_Ranks == 0 && (i + X_Axis < Start_Image_Rows)) || ((Processors_Ranks == Processors_Size - 1) && (i - Start_Image_Rows + X_Axis >= Work_Rows))) {
						Y_Axis++;
						continue;
					}

					New_Value += Kernel_Filter[m][l] * Image_Array[(i + X_Axis) * Current_Image_Width + (j + Y_Axis)];
					Y_Axis++;
				}
				X_Axis++; Y_Axis = -(Kernel_Size / 2);
			}
			Work_Image_Array[(i - Start_Image_Rows) * Current_Image_Width + j] = New_Value;///lesa
		}
	}


	if (Processors_Ranks == Lower_Rank)
	{
		// Define the receive counts
		int* Work_Size_Counts = new int[Processors_Size];
		int* Displacement_Size = new int[Processors_Size];
		for (int i = 0; i < Processors_Size; i++)
		{
			if (i == Processors_Size - 1)
			{
				Work_Size_Counts[i] = (Current_Image_Width * (Current_Image_Height)) - (Work_Size * (Final_Rank));

				Displacement_Size[i] = (i)*Work_Size;

				break;

			}
			Displacement_Size[i] = i * Work_Size;
			Work_Size_Counts[i] = Work_Size;
		}
		MPI_Gatherv(Work_Image_Array, Work_Size, MPI_FLOAT, Image_Array, Work_Size_Counts, Displacement_Size, MPI_FLOAT, Lower_Rank, MPI_COMM_WORLD);
	}
	else
	{
		MPI_Gatherv(Work_Image_Array, Work_Size, MPI_FLOAT, NULL, NULL, NULL, MPI_FLOAT, Lower_Rank, MPI_COMM_WORLD);
	}
	if (Lower_Rank) {
		
		End_Time = clock();
		Full_Time += (End_Time - Start_Time) / double(CLOCKS_PER_SEC) * 1000;
		
		cout << "Time: " << Full_Time << endl;

		Draw_New_Image(Image_Array, Current_Image_Width, Current_Image_Height);
	}


	MPI_Finalize();//////////////////////////////////////////////////////////////////
	///end


	free(Image_Array);
	return 0;

}