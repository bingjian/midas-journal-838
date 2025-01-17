#include "CBCTSplineDrivenImageSlicer.h"
#include "CBCTFrenetSerretFrame.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkImageReslice.h"


#include "vtkPlaneSource.h"
#include "vtkImageData.h"
#include "vtkProbeFilter.h"
#include "vtkMatrix4x4.h"
#include "vtkImageAppend.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <iostream>

vtkStandardNewMacro(CBCTSplineDrivenImageSlicer);

CBCTSplineDrivenImageSlicer::CBCTSplineDrivenImageSlicer()
{
    this->localFrenetFrames = CBCTFrenetSerretFrame::New();
    this->reslicer = vtkImageReslice::New();
    this->SliceExtent[0] = 256;
    this->SliceExtent[1] = 256;
    this->SliceSpacing[0] = 0.25;
    this->SliceSpacing[1] = 0.25;
    this->SliceThickness = 0.25;
    this->OffsetPoint = 0;
    this->OffsetLine = 0;
    this->ProbeInput = 0;
    this->Binormal[0] = 0;
    this->Binormal[1] = 0;
    this->Binormal[2] = 1;

    this->SetNumberOfInputPorts(2);
    this->SetNumberOfOutputPorts(2);

    // by default process active point scalars
    this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS);
}

CBCTSplineDrivenImageSlicer::~CBCTSplineDrivenImageSlicer()
{
    this->localFrenetFrames->Delete();
    this->reslicer->Delete();
}

//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void CBCTSplineDrivenImageSlicer::SetPathConnection(int id, vtkAlgorithmOutput* algOutput)
{
    if (id < 0)
    {
        vtkErrorMacro("Bad index " << id << " for source.");
        return;
    }

    int numConnections = this->GetNumberOfInputConnections(1);
    if (id < numConnections)
    {
        this->SetNthInputConnection(1, id, algOutput);
    }
    else if (id == numConnections && algOutput)
    {
        this->AddInputConnection(1, algOutput);
    }
    else if (algOutput)
    {
        vtkWarningMacro("The source id provided is larger than the maximum "
            "source id, using " << numConnections << " instead.");
        this->AddInputConnection(1, algOutput);
    }
}

//---------------------------------------------------------------------------
int CBCTSplineDrivenImageSlicer::FillInputPortInformation(int port, vtkInformation* info)
{
    if (port == 0)
    {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    }
    else
    {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    }
    return 1;
}

//----------------------------------------------------------------------------
int CBCTSplineDrivenImageSlicer::FillOutputPortInformation(
    int port, vtkInformation* info)
{
    if (port == 0)
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    if (port == 1)
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
}


int CBCTSplineDrivenImageSlicer::RequestInformation(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector**,
    vtkInformationVector* outputVector)
{

    // get the info objects
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    int extent[6] = { 0, this->SliceExtent[0] - 1,
                     0, this->SliceExtent[1] - 1,
                     0, 1 };
    double spacing[3] = { this->SliceSpacing[0],this->SliceSpacing[1],this->SliceThickness };


    outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        extent, 6);

    return 1;
}

//! RequestData is called by the pipeline process. 
int CBCTSplineDrivenImageSlicer::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
    // get the info objects
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* pathInfo = inputVector[1]->GetInformationObject(0);
    vtkInformation* outImageInfo = outputVector->GetInformationObject(0);
    vtkInformation* outPlaneInfo = outputVector->GetInformationObject(1);

    // get the input and ouptut
    vtkImageData* input = vtkImageData::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkSmartPointer<vtkImageData> inputCopy = vtkSmartPointer<vtkImageData>::New();
    inputCopy->ShallowCopy(input);
    vtkPolyData* inputPath = vtkPolyData::SafeDownCast(
        pathInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkImageData* outputImage = vtkImageData::SafeDownCast(
        outImageInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkPolyData* outputPlane = vtkPolyData::SafeDownCast(
        outPlaneInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkSmartPointer<vtkPolyData> pathCopy = vtkSmartPointer<vtkPolyData>::New();
    pathCopy->ShallowCopy(inputPath);


    // Compute the local normal and tangent to the path
    this->localFrenetFrames->SetInputData(pathCopy);
    this->localFrenetFrames->SetViewUp(this->Incidence);
    this->localFrenetFrames->ComputeBinormalOn();
    this->localFrenetFrames->Update();

    // path will contain PointData array "Tangents" and "Vectors"
    vtkPolyData* path = static_cast<vtkPolyData*>
        (this->localFrenetFrames->GetOutputDataObject(0));
    // Count how many points are used in the cells
    // In case of loop, points may be used several times
    // (note: not using NumberOfPoints because we want only LINES points...)
    vtkCellArray* lines = path->GetLines();
    lines->InitTraversal();

    vtkIdType nbCellPoints;
#ifdef VTK_CELL_ARRAY_V2
    const vtkIdType* points;
#else // VTK_CELL_ARRAY_V2
    vtkIdType* points;
#endif // VTK_CELL_ARRAY_V2

    vtkIdType cellId = -1;
    do {
        lines->GetNextCell(nbCellPoints, points);
        cellId++;
    } while (cellId != this->OffsetLine);

    int ptId = this->OffsetPoint;
    if (ptId >= nbCellPoints)
    {
        ptId = nbCellPoints - 1;
    }
    // Build a new reslicer with image input as input too.
    this->reslicer->SetInputData(inputCopy);

    // Get the Frenet-Serret chart at point ptId:
    // - position (center)
    // - tangent T
    // - normal N
    double center[3];
    path->GetPoints()->GetPoint(ptId, center);
    vtkDoubleArray* pathTangents = static_cast<vtkDoubleArray*>
        (path->GetPointData()->GetArray("FSTangents"));
    double tangent[3];
    pathTangents->GetTuple(ptId, tangent);

    vtkDoubleArray* pathNormals = static_cast<vtkDoubleArray*>
        (path->GetPointData()->GetArray("FSNormals"));
    double normal[3];
    pathNormals->GetTuple(ptId, normal);

    vtkDoubleArray* pathBinormals = static_cast<vtkDoubleArray*>
        (path->GetPointData()->GetArray("FSBinormals"));


    //double binormal[3];
    //pathBinormals->GetTuple(ptId, binormal);

    //double crossProduct[3];
    //vtkMath::Cross(tangent, normal, crossProduct);

    // Build the plane output that will represent the slice location in 3D view
    vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();

    double planeOrigin[3];
    double planePoint1[3];
    double planePoint2[3];
    for (int comp = 0; comp < 3; comp++)
    {
        planeOrigin[comp] = center[comp] - normal[comp] * this->SliceExtent[1] * this->SliceSpacing[1] / 2.0
            - this->Binormal[comp] * this->SliceExtent[0] * this->SliceSpacing[0] / 2.0;
        planePoint1[comp] = planeOrigin[comp] + this->Binormal[comp] * this->SliceExtent[0] * this->SliceSpacing[0];
        planePoint2[comp] = planeOrigin[comp] + normal[comp] * this->SliceExtent[1] * this->SliceSpacing[1];
    }
    plane->SetOrigin(planeOrigin);
    plane->SetPoint1(planePoint1);
    plane->SetPoint2(planePoint2);
    plane->SetResolution(this->SliceExtent[0],
        this->SliceExtent[1]);
    plane->Update();

    if (this->ProbeInput == 1)
    {
        vtkSmartPointer<vtkProbeFilter> probe = vtkSmartPointer<vtkProbeFilter>::New();
        probe->SetInputConnection(plane->GetOutputPort());
        probe->SetSourceData(inputCopy);
        probe->Update();
        outputPlane->DeepCopy(probe->GetOutputDataObject(0));
    }
    else
    {
        outputPlane->DeepCopy(plane->GetOutputDataObject(0));
    }

    // Build the transformation matrix (inspired from vtkImagePlaneWidget)
    vtkMatrix4x4* resliceAxes = vtkMatrix4x4::New();
    resliceAxes->Identity();
    double origin[4];
    // According to vtkImageReslice API:
    // - 1st column contains the resliced image x-axis
    // - 2nd column contains the resliced image y-axis
    // - 3rd column contains the normal of the resliced image plane
    // -> 1st column is normal to the path
    // -> 3nd column is tangent to the path
    // -> 2nd column is B = T^N
    for (int comp = 0; comp < 3; comp++)
    {
        resliceAxes->SetElement(0, comp, this->Binormal[comp]);
        resliceAxes->SetElement(1, comp, normal[comp]);
        resliceAxes->SetElement(2, comp, tangent[comp]);

        origin[comp] = center[comp] - normal[comp] * this->SliceExtent[1] * this->SliceSpacing[1] / 2.0
            - this->Binormal[comp] * this->SliceExtent[0] * this->SliceSpacing[0] / 2.0;
    }

    //! Transform the origin in the homogeneous coordinate space. 
    //! \todo See why !
    origin[3] = 1.0;
    double originXYZW[4];
    resliceAxes->MultiplyPoint(origin, originXYZW);

    //! Get the new origin from the transposed matrix. 
    //! \todo See why !
    resliceAxes->Transpose();
    double neworiginXYZW[4];
    resliceAxes->MultiplyPoint(originXYZW, neworiginXYZW);

    resliceAxes->SetElement(0, 3, neworiginXYZW[0]);
    resliceAxes->SetElement(1, 3, neworiginXYZW[1]);
    resliceAxes->SetElement(2, 3, neworiginXYZW[2]);


    this->reslicer->SetResliceAxes(resliceAxes);
    this->reslicer->SetInformationInput(input);
    this->reslicer->SetInterpolationModeToCubic();

    this->reslicer->SetOutputDimensionality(2);
    this->reslicer->SetOutputOrigin(0, 0, 0);
    this->reslicer->SetOutputExtent(0, this->SliceExtent[0] - 1,
        0, this->SliceExtent[1] - 1,
        0, 300);
    this->reslicer->SetOutputSpacing(this->SliceSpacing[0],
        this->SliceSpacing[1],
        this->SliceThickness);
    this->reslicer->Update();

    resliceAxes->Delete();


    outputImage->DeepCopy(this->reslicer->GetOutputDataObject(0));
    outputImage->GetPointData()->GetScalars()->SetName("ReslicedImage");

    return 1;

}