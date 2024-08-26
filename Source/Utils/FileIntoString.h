#ifndef SHADER_AS_STRING_H
#define SHADER_AS_STRING_H

// Reads a file into an std::string 
std::string ReadFileIntoString(const char* filePath)
{
	std::string output;
	unsigned int stringLength = 0;
	GW::SYSTEM::GFile file;

	/*char* outdir = new char[80];
	unsigned int outdirStringLength = 0;
	file.GetCurrentWorkingDirectory(outdir, outdirStringLength);
	file.SetCurrentWorkingDirectory("../../Shaders/");*/

	file.Create(); // this is where the interface unsupported return happens - needs fixing
	file.GetFileSize(filePath, stringLength);

	if (stringLength > 0 && +file.OpenBinaryRead(filePath))
	{
		output.resize(stringLength);
		file.Read(&output[0], stringLength);
	}
	else
		std::cout << "ERROR: File \"" << filePath << "\" Not Found!" << std::endl;

	return output;
}

#endif
