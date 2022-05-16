#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* Properties_Image(int* w, int* h, System::String^ Image_Location) //put the size of image in w & h
{
	int* Enter_Input;


	int Input_Image_Width, Input_Image_Height;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap Bitmap_Array(Image_Location);

	Input_Image_Width = Bitmap_Array.Width;
	Input_Image_Height = Bitmap_Array.Height;
	*w = Bitmap_Array.Width;
	*h = Bitmap_Array.Height;
	int *Red_Channel = new int[Bitmap_Array.Height * Bitmap_Array.Width];
	int *Green_Channel = new int[Bitmap_Array.Height * Bitmap_Array.Width];
	int *Blue_Channel = new int[Bitmap_Array.Height * Bitmap_Array.Width];
	Enter_Input = new int[Bitmap_Array.Height*Bitmap_Array.Width];
	
	//Image Sizes
	int Image_Height = Bitmap_Array.Height;
	int Image_Width = Bitmap_Array.Width;
	int Height_Count = 0;
	int Width_Count = 0;

	while (Height_Count < Image_Height)
	{
		while (Width_Count < Image_Width)
		{
			System::Drawing::Color Color_Values = Bitmap_Array.GetPixel(Width_Count, Height_Count);

			Red_Channel[Height_Count * Bitmap_Array.Width + Width_Count] = Color_Values.R;
			Blue_Channel[Height_Count * Bitmap_Array.Width + Width_Count] = Color_Values.B;
			Green_Channel[Height_Count * Bitmap_Array.Width + Width_Count] = Color_Values.G;

			//gray scale value equals the average of RGB values
			Enter_Input[Height_Count * Bitmap_Array.Width + Width_Count] = ((Color_Values.R + Color_Values.B + Color_Values.G) / 3); 

			Width_Count++;
		}
		Width_Count = 0;
		Height_Count++;
	}
	return Enter_Input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap Create_New_Image(width, height);

	//Image Sizes
	int Image_Height = Create_New_Image.Height;
	int Image_Width = Create_New_Image.Width;
	int Height_Count = 0;
	int Width_Count = 0;

	while (Height_Count < Image_Height)
	{
		while (Width_Count < Image_Width)
		{
			//i * OriginalImageWidth + j
			if (image[Height_Count * width + Width_Count] < 0)
			{
				image[Height_Count * width + Width_Count] = 0;
			}
			if (image[Height_Count * width + Width_Count] > 255)
			{
				image[Height_Count * width + Width_Count] = 255;
			}
			System::Drawing::Color Color_Values = System::Drawing::Color::FromArgb(image[Height_Count * Create_New_Image.Width + Width_Count], image[Height_Count * Create_New_Image.Width + Width_Count], image[Height_Count * Create_New_Image.Width + Width_Count]);
			Create_New_Image.SetPixel(Width_Count, Height_Count, Color_Values);
			Width_Count++;
		}
		Width_Count = 0;
		Height_Count++;
	}

	//Create and Save the new image
	Create_New_Image.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}


int main()
{
	int Current_Image_Width = 4, Current_Image_Height = 4;

	int Begin_Time, End_Time, Full_Time = 0;

	MPI_Init(NULL, NULL);
	int Number_Processors, rank;
	MPI_Comm_size(MPI_COMM_WORLD, &Number_Processors); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); 

	System::String^ Image_Direction;
	std::string Image_Location;
	Image_Location = "..//Data//Input//test.png";

	Image_Direction = marshal_as<System::String^>(Image_Location);
	int* Image_Values = Properties_Image(&Current_Image_Width, &Current_Image_Height, Image_Direction);


	Begin_Time = clock();
	createImage(Image_Values, Current_Image_Width, Current_Image_Height, 0);
	End_Time = clock();
	Full_Time += (End_Time - Begin_Time) / double(CLOCKS_PER_SEC) * 1000;
	cout << "time: " << Full_Time << endl;

	free(Image_Values);
	return 0;

}



