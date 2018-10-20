#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkSphereSource.h>
#include <vtkMetaImageWriter.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include <vtkPointData.h>
#include <vtkSTLReader.h>
#include <iostream>
#include <vector>
#include "dataIO.h"

int main(int argc, char *argv[])
{
	if (argc != 12) {
		std::cerr << "usage: [.exe] [input stl file list] [output dir] [image size x] "
			<< "[image size y] [image size z] [resolution x] [resolution y] "
			<< "[resolution z] [center x] [center y] [center z]" << std::endl;
		return EXIT_FAILURE;
	}

	const std::string IPath = argv[1];			//input stl file list (txt)
	const std::string OPath = argv[2];			//output dir
	const vtkIdType xe = std::stoi(argv[3]);	//image size of voxel data
	const vtkIdType ye = std::stoi(argv[4]);	
	const vtkIdType ze = std::stoi(argv[5]);
	const double xr = std::stof(argv[6]);		//resolution of voxel data
	const double yr = std::stof(argv[7]);
	const double zr = std::stof(argv[8]);
	const double x0 = std::stoi(argv[9]);		//center of voxel data
	const double y0 = std::stoi(argv[10]);
	const double z0 = std::stoi(argv[11]);

	int data_num;

	std::list<std::string> file_path_list;
	std::string output_file_mhd;
	//std::string output_file_raw;

	dataIO::check_folder(OPath + "//");
	data_num = dataIO::count_number_of_text_lines(IPath);
	dataIO::get_list_txt(file_path_list, IPath, data_num);

	///* change to vector */
	std::vector<std::string> data_path(file_path_list.begin(), file_path_list.end());

	for (int i = 0; i < data_path.size(); ++i) {

		output_file_mhd = OPath + "/voxel_data_" + std::to_string(i+1) + ".mhd";
		//output_file_raw = OPath + "/surface_fitted_" + std::to_string(i + 1) + ".raw";
		//output_file_mhd = OPath + "/generate_" + std::to_string(i+1) + ".mhd";
		//output_file_mhd = OPath + "/reconstruction_" + std::to_string(i + 1) + ".mhd";

		std::cout << "Write->" << output_file_mhd << std::endl;

		vtkSmartPointer<vtkSTLReader> reader =
			vtkSmartPointer<vtkSTLReader>::New();
		reader->SetFileName(data_path[i].c_str());
		reader->Update();

		vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
		pd->DeepCopy(reader->GetOutput());

		vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
		imageData->SetDimensions(xe, ye, ze);
		imageData->SetSpacing(xr, yr, zr);
		imageData->SetOrigin(x0, y0, z0);
		imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

		// fill the image with foreground voxels:
		unsigned char inval = 1;
		unsigned char outval = 0;
		vtkIdType count = imageData->GetNumberOfPoints();
		for (vtkIdType i = 0; i < count; ++i)
		{
			imageData->GetPointData()->GetScalars()->SetTuple1(i, inval);
		}

		// polygonal data --> image stencil:
		vtkSmartPointer<vtkPolyDataToImageStencil> pol2stenc =
			vtkSmartPointer<vtkPolyDataToImageStencil>::New();
#if VTK_MAJOR_VERSION <= 5
		pol2stenc->SetInput(pd);
#else
		pol2stenc->SetInputData(pd);
#endif
		pol2stenc->SetOutputOrigin(x0, y0, z0);
		pol2stenc->SetOutputSpacing(xr, yr, zr);
		pol2stenc->SetOutputWholeExtent(imageData->GetExtent());
		pol2stenc->Update();

		// cut the corresponding white image and set the background:
		vtkSmartPointer<vtkImageStencil> imgstenc =
			vtkSmartPointer<vtkImageStencil>::New();

#if VTK_MAJOR_VERSION <= 5
		imgstenc->SetInput(whiteImage);
		imgstenc->SetStencil(pol2stenc->GetOutput());
#else
		imgstenc->SetInputData(imageData);
		imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
#endif
		imgstenc->ReverseStencilOff();
		imgstenc->SetBackgroundValue(outval);
		imgstenc->Update();

		vtkSmartPointer<vtkMetaImageWriter> writer =
			vtkSmartPointer<vtkMetaImageWriter>::New();
		writer->SetFileName(output_file_mhd.c_str());
		//writer->SetRAWFileName(output_file_raw.c_str());
#if VTK_MAJOR_VERSION <= 5
		writer->SetInput(imgstenc->GetOutput());
#else
		writer->SetInputData(imgstenc->GetOutput());
#endif
		writer->Write();
	}

	return EXIT_SUCCESS;
}