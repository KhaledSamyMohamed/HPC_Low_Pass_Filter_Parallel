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

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}

void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
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
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}


	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

float** Calculate_Kernel(int Kernel_Size) {
	float** Kernellll = new float* [Kernel_Size];
	int While_Count = 0;
	while (While_Count < Kernel_Size)
	{
		Kernellll[While_Count] = new float[Kernel_Size];
		for (int i = 0; i < Kernel_Size; i++)
		{
			Kernellll[While_Count][i] = 1.0 / (Kernel_Size * Kernel_Size);

		}
		While_Count++;
	}
	return Kernellll;
}

int* KernelComp(int Start_Work_Rows, int End_Work_Rows, int Current_Image_Width, int Kernel_Size, int Processors_ID, int Last_Processor, int Work_Rows, float** Kernel_Filter, int* Image_Array, int* Work_Image_Array)
{
	int x = Start_Work_Rows;
	//loop thourgh img rows
	while (x < End_Work_Rows)
	{	//loop thourgh img columns
		for (int y = 0; y < Current_Image_Width; y++)
		{
			int New_Value = 0;
			//divide kernel into two half
			int X_Axis = -(Kernel_Size / 2);
			int Y_Axis = -(Kernel_Size / 2);
			//loop thourgh kern rows
			for (int xx = 0; xx < Kernel_Size; xx++)
			{	//loop thourgh kernel columns
				for (int yy = 0; yy < Kernel_Size; yy++)
				{
					//check if if kernel is outside the photo --->ignore
					if ((y + Y_Axis < 0) || (y + Y_Axis >= Current_Image_Width)) {//columns
						Y_Axis++;
						continue;
					}
					//ignore the first and last free places from applying the filter
					if (((Processors_ID == Last_Processor) && (x - Start_Work_Rows + X_Axis >= Work_Rows)) || (Processors_ID == 0 && (x + X_Axis < Start_Work_Rows))) {
						Y_Axis++;
						continue;
					}
					// multiply filter to image 
					New_Value += Image_Array[(x + X_Axis) * Current_Image_Width + (y + Y_Axis)] * Kernel_Filter[xx][yy];
					Y_Axis++;
				}
				X_Axis++;
				Y_Axis = -(Kernel_Size / 2);
			}
			Work_Image_Array[(x - Start_Work_Rows) * Current_Image_Width + y] = New_Value;
		}
		x++;
	}

	return Work_Image_Array;
}

int main()
{
	int Current_Image_Width = 4, Current_Image_Height = 4;

	int Start_Time, End_Time, Full_Time = 0;

	System::String^ Image_Location;
	std::string Image;
	Image = "..//Data//Input//N.png";

	Image_Location = marshal_as<System::String^>(Image);
	int* Image_Array = inputImage(&Current_Image_Width, &Current_Image_Height, Image_Location); //pixel values of image

	/////////How To Run/////////
	//////mpiexec "HPC_ProjectTemplate.exe"//////
	// 
	////////////Start////////////

	MPI_Init(NULL, NULL);
	int Processors_ID;
	MPI_Comm_rank(MPI_COMM_WORLD, &Processors_ID);
	int Processors_Size;
	MPI_Comm_size(MPI_COMM_WORLD, &Processors_Size);

	//define Lower and Last Processor
	int Lower_Processor = 0;
	int Last_Processor = Processors_Size - 1;

	float** Kernel_Filter;
	int Kernel_Size;
	//Take the kernel size
	// 
	if (Processors_ID == Lower_Processor) {

		cout << "Please Enter The Kernel Size (ODD Number): ";
		cin >> Kernel_Size;
		if (Kernel_Size % 2 == 0)
		{
			Kernel_Size -= 1;
		}
	}

	//Broadcast kernel size to all processors
	MPI_Bcast(&Kernel_Size, 1, MPI_INT, 0, MPI_COMM_WORLD);

	//Calculate the Kernel
	Kernel_Filter = Calculate_Kernel(Kernel_Size);

	//Start Time
	Start_Time = clock();

	//Define the Divided Row for each processor 
	int Divided_Rows = Current_Image_Height / Processors_Size;
	int Start_Work_Rows = Divided_Rows * Processors_ID;
	int End_Work_Rows = Start_Work_Rows + Divided_Rows;

	//last processor take the excced Pixels
	if (Processors_ID == Last_Processor)
	{
		End_Work_Rows = Current_Image_Height;
	}

	cout << Processors_ID << ' ' << Start_Work_Rows << ' ' << End_Work_Rows << ' ' << endl;

	//Define the work Rows 
	int Work_Rows = End_Work_Rows - Start_Work_Rows;
	int Work_Size = (Work_Rows)*Current_Image_Width;
	int* Work_Image_Array = new int[Work_Size];
	int* workImageArray = new int[Work_Size];

	//Kernel Calculation and save the data
	Work_Image_Array = KernelComp(Start_Work_Rows, End_Work_Rows, Current_Image_Width, Kernel_Size, Processors_ID, Last_Processor, Work_Rows, Kernel_Filter, Image_Array, workImageArray);

	// Define the receive counts and gather the new data
	if (Processors_ID == Lower_Processor)
	{
		int While_Count = 0;
		int* Processors = new int[Processors_Size];
		int* Processors_Work_Size = new int[Processors_Size];
		while (While_Count < Processors_Size)
		{
			if (While_Count == Last_Processor)
			{
				//if last Proccessor -> make sure to deal with last pixels 
				Processors[While_Count] = (Current_Image_Width * (Current_Image_Height)) - (Work_Size * (Last_Processor));

				Processors_Work_Size[While_Count] = (While_Count)*Work_Size;

				break;

			}
			//
			Processors_Work_Size[While_Count] = While_Count * Work_Size;
			Processors[While_Count] = Work_Size;
			While_Count++;
		}
		
		/*int MPI_Gatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
			void* recvbuf, const int* recvcounts, const int* displs,
			MPI_Datatype recvtype, int root, MPI_Comm comm)*/

		MPI_Gatherv(Work_Image_Array, Work_Size, MPI_FLOAT, Image_Array, Processors, Processors_Work_Size, MPI_FLOAT, Lower_Processor, MPI_COMM_WORLD);
	}
	else
	{
		/*MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
			void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)*/

		/*MPI_Gather(Work_Image_Array, Work_Size, MPI_FLOAT, NULL, NULL, MPI_FLOAT, Lower_Processor, MPI_COMM_WORLD);*/

		/*int MPI_Gatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
			void* recvbuf, const int* recvcounts, const int* displs,
			MPI_Datatype recvtype, int root, MPI_Comm comm)*/

		MPI_Gatherv(Work_Image_Array, Work_Size, MPI_FLOAT, NULL, NULL, NULL, MPI_FLOAT, Lower_Processor, MPI_COMM_WORLD);
	}

	//Calculate Time and Create New Image
	if (Processors_ID == 0) {
		
		End_Time = clock();
		Full_Time += (End_Time - Start_Time) / double(CLOCKS_PER_SEC) * 1000;

		cout << "Time: " << Full_Time << endl;

		createImage(Image_Array, Current_Image_Width, Current_Image_Height, 10);
	}

	MPI_Finalize();

	/////////end/////////
	
	//Free Memory
	free(Image_Array);

	return 0;
}