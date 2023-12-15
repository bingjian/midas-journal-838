#ifndef CBCTSplineDrivenImageSlicer_h
#define CBCTSplineDrivenImageSlicer_h

#include"vtkImageAlgorithm.h"

class CBCTFrenetSerretFrame;
class vtkImageReslice;

#include "SplineDrivenImageSlicerModule.h" // For export macro

class SPLINEDRIVENIMAGESLICER_EXPORT CBCTSplineDrivenImageSlicer : public vtkImageAlgorithm
{
public:
    vtkTypeMacro(CBCTSplineDrivenImageSlicer, vtkImageAlgorithm);
    static CBCTSplineDrivenImageSlicer* New();

    //! Specify the path represented by a vtkPolyData wich contains PolyLines
    void SetPathConnection(int id, vtkAlgorithmOutput* algOutput);
    void SetPathConnection(vtkAlgorithmOutput* algOutput)
    {
        this->SetPathConnection(0, algOutput);
    };
    vtkAlgorithmOutput* GetPathConnection()
    {
        return(this->GetInputConnection(1, 0));
    };

    vtkSetVector2Macro(SliceExtent, int);
    vtkGetVector2Macro(SliceExtent, int);

    vtkSetVector2Macro(SliceSpacing, double);
    vtkGetVector2Macro(SliceSpacing, double);

    vtkSetMacro(SliceThickness, double);
    vtkGetMacro(SliceThickness, double);

    vtkSetMacro(OffsetPoint, vtkIdType);
    vtkGetMacro(OffsetPoint, vtkIdType);

    vtkSetMacro(OffsetLine, vtkIdType);
    vtkGetMacro(OffsetLine, vtkIdType);

    vtkSetMacro(ProbeInput, vtkIdType);
    vtkGetMacro(ProbeInput, vtkIdType);
    vtkBooleanMacro(ProbeInput, vtkIdType);

    vtkSetMacro(Incidence, double);
    vtkGetMacro(Incidence, double);

    vtkSetVector3Macro(Binormal, double);
    vtkGetVector3Macro(Binormal, double);


protected:
    CBCTSplineDrivenImageSlicer();
    ~CBCTSplineDrivenImageSlicer();

    virtual int RequestData(vtkInformation*, vtkInformationVector**,
        vtkInformationVector*) override;

    virtual int FillInputPortInformation(int port, vtkInformation* info) override;
    virtual int FillOutputPortInformation(int, vtkInformation*) override;
    virtual int RequestInformation(vtkInformation*, vtkInformationVector**,
        vtkInformationVector*) override;
private:
    CBCTSplineDrivenImageSlicer(const CBCTSplineDrivenImageSlicer&) = delete;
    void operator=(const CBCTSplineDrivenImageSlicer&) = delete;

    CBCTFrenetSerretFrame* localFrenetFrames; //!< computes local tangent along path input
    vtkImageReslice* reslicer; //!< Reslicers array

    int     SliceExtent[2]; //!< Number of pixels nx, ny in the slice space around the center points
    double SliceSpacing[2]; //!< Pixel size sx, sy of the output slice
    double SliceThickness; //!< Slice thickness (useful for volumic reconstruction) 
    double Incidence; //!< Rotation of the initial normal vector.
    double Binormal[3]; //!< ÇÐÃæ·½Ïò

    vtkIdType OffsetPoint; //!< Id of the point where the reslicer proceed
    vtkIdType OffsetLine; //!< Id of the line cell where to get the reslice center
    vtkIdType ProbeInput; //!< If true, the output plane (2nd output probes the input image)
};

#endif //__CBCTSplineDrivenImageSlicer_h__