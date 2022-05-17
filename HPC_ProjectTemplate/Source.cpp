#include <iostream>
#include <math.h>
#include <mpi.h> ////
#include <stdio.h>////
#include <stdlib.h>
#include <string.h>
#include <msclr\marshal_cppstd.h>
#include <ctime>// include this header 
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

int main(int args, char** argv)
{
	MPI_Init(NULL, NULL);
	int Number_Processors, Rank_Id;
	MPI_Comm_size(MPI_COMM_WORLD, &Number_Processors); //processors number
	MPI_Comm_rank(MPI_COMM_WORLD, &Rank_Id); //processor id
	int Current_Image_Width = 4, Current_Image_Height = 4; //imgwidth=cols ,,imgheight=rows
	int* Image_Array_Values = NULL; //1d img data
	int Spilt_Lenght, Begin_Send, End_Send, Recived_Counter, Recived_Array_Start, Reminder_Spilt;
	int* Recived_Array = NULL;//contain sub send data
	MPI_Status Begin;
	// start get img data
	System::String^ Image_Direction;
	std::string Image_Location;
	Image_Location = "..//Data//Input//test.png";     //img path
	Image_Direction = marshal_as<System::String^>(Image_Location);
	Image_Array_Values = inputImage(&Current_Image_Width, &Current_Image_Height, Image_Direction);
	// end get img data
	if (Rank_Id == 0)
	{
		//start split the data	1darr
		Spilt_Lenght = (Current_Image_Height / Number_Processors) * Current_Image_Width; //need to be send to other process //kam height
		Reminder_Spilt = (Current_Image_Height % Number_Processors) * Current_Image_Width;
		for (int i = 1; i < Number_Processors; i++)
		{
			Begin_Send = i * Spilt_Lenght; //zero index 
			if (i == Number_Processors - 1) Spilt_Lenght += Reminder_Spilt;
			Recived_Array_Start = Current_Image_Width;
			MPI_Send(&Spilt_Lenght, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&Current_Image_Height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&Current_Image_Width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&Recived_Array_Start, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			if (i == Number_Processors - 1)
			{
				Recived_Counter = Spilt_Lenght + Current_Image_Width;
				MPI_Send(&Recived_Counter, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&Image_Array_Values[Begin_Send - Current_Image_Width], Recived_Counter, MPI_INT, i, 0, MPI_COMM_WORLD);
			}
			else
			{
				Recived_Counter = Spilt_Lenght + Current_Image_Width * 2;
				MPI_Send(&Recived_Counter, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				MPI_Send(&Image_Array_Values[Begin_Send - Current_Image_Width], Recived_Counter, MPI_INT, i, 0, MPI_COMM_WORLD);
			}
		}
		//process 0 data
		Spilt_Lenght = (Current_Image_Height / Number_Processors) * Current_Image_Width;
		if (Number_Processors > 1)
			Recived_Counter = Spilt_Lenght + Current_Image_Width;
		else
			Recived_Counter = Spilt_Lenght;
		Recived_Array_Start = 0;
		Recived_Array = new int[Recived_Counter];
		for (int i = 0; i < Recived_Counter; i++)
			Recived_Array[i] = Image_Array_Values[i];
		//end split the data 1darr
	}
	else if (Rank_Id < Number_Processors)  //start recv data 
	{
		MPI_Recv(&Spilt_Lenght, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Begin);
		MPI_Recv(&Current_Image_Height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Begin);
		MPI_Recv(&Current_Image_Width, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Begin);
		MPI_Recv(&Recived_Array_Start, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Begin);
		MPI_Recv(&Recived_Counter, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Begin);
		Recived_Array = new int[Recived_Counter];
		MPI_Recv(Recived_Array, Recived_Counter, MPI_INT, 0, 0, MPI_COMM_WORLD, &Begin);
	} //end recv data
	//start process code
	int* Recived_Array_Sum = new int[Spilt_Lenght + Reminder_Spilt];//for answer data
	double Local_Summ = 0;
	///
	int positions[9], incIndex = 0;
	///
	End_Send = Spilt_Lenght + Current_Image_Width;
	if (Rank_Id == 0)
		End_Send = Spilt_Lenght;
	for (int i = Recived_Array_Start; i < End_Send; i++)
	{
		positions[4] = Recived_Array[i];	//center point
		if (i - Current_Image_Width < 0) //upper center point
			positions[1] = Recived_Array[i]; //mfe4 upper center
		else
			positions[1] = Recived_Array[i - Current_Image_Width]; //fe upper center
		if (i + Current_Image_Width >= Recived_Counter) //down center point
			positions[7] = Recived_Array[i]; //mfe4 down center
		else
			positions[7] = Recived_Array[i + Current_Image_Width]; //fe down center
		if (i % Current_Image_Width == 0)	//left point
		{
			positions[3] = positions[4]; //mfe4 left
			positions[0] = positions[4];//mfe4 upper left
			positions[6] = positions[7]; //mfe4 down left
		}
		else
		{
			positions[3] = Recived_Array[i - 1]; //fe left
			if (i - 1 - Current_Image_Width < 0) //upper left point
				positions[0] = positions[4]; //mfe4 upper left
			else
				positions[0] = Recived_Array[i - 1 - Current_Image_Width]; //fe upper left
			if (i - 1 + Current_Image_Width >= Recived_Counter) //down left point
				positions[6] = Recived_Array[i]; //mfe4 down left
			else
				positions[6] = Recived_Array[i - 1 + Current_Image_Width]; //fe down left
		}
		if ((i + 1) % Current_Image_Width == 0)	//right  point
		{
			positions[5] = positions[4]; //mfe4 right
			positions[2] = positions[4];//mfe4 upper right
			positions[8] = positions[7]; //mfe4 down right
		}
		else
		{
			positions[5] = Recived_Array[i + 1]; //fe right
			if (i + 1 - Current_Image_Width < 0) //upper right point
				positions[2] = positions[4]; //mfe4 upper right
			else
				positions[2] = Recived_Array[i + 1 - Current_Image_Width]; //fe upper right
			if (i + 1 + Current_Image_Width >= Recived_Counter) //down right point
				positions[8] = Recived_Array[i]; //mfe4 down right
			else
				positions[8] = Recived_Array[i + 1 + Current_Image_Width]; //fe down right
		}
		for (int j = 0; j < 9; j++)      //divide by 9 try change it change the values and the photo
			Local_Summ += positions[j] / 9;
		Recived_Array_Sum[incIndex++] = Local_Summ;
		Local_Summ = 0;
	}
	if (Rank_Id > 0)//send answer data
	{
		MPI_Send(&Spilt_Lenght, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(Recived_Array, Spilt_Lenght, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}
	if (Rank_Id == 0)//collect data
	{
		int inc = 0;
		for (int i = 0; i < Spilt_Lenght; i++)
			Image_Array_Values[inc++] = Recived_Array_Sum[i];
		for (int j = 1; j < Number_Processors; j++)
		{
			MPI_Recv(&Spilt_Lenght, 1, MPI_INT, j, 0, MPI_COMM_WORLD, &Begin);//rank j
			MPI_Recv(Recived_Array_Sum, Spilt_Lenght, MPI_INT, j, 0, MPI_COMM_WORLD, &Begin);//rank j
			for (int i = 0; i < Spilt_Lenght; i++)
				Image_Array_Values[inc++] = Recived_Array_Sum[i];
		}
		//make img
			//start clock times
		int Begin_Time, End_Time, Complete_Time = 0;
		Begin_Time = clock();
		createImage(Image_Array_Values, Current_Image_Width, Current_Image_Height, 0);
		End_Time = clock();
		Complete_Time += (End_Time - Begin_Time) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time: " << Complete_Time << endl;
		free(Image_Array_Values);
	}
	MPI_Finalize();
	return 0;
}