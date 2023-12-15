#ifndef CBCTFrenetSerretFrame_h
#define CBCTFrenetSerretFrame_h

#include "SplineDrivenImageSlicerModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class SPLINEDRIVENIMAGESLICER_EXPORT CBCTFrenetSerretFrame : public vtkPolyDataAlgorithm
{
public:
    vtkTypeMacro(CBCTFrenetSerretFrame, vtkPolyDataAlgorithm);
    static CBCTFrenetSerretFrame* New();

    //! Set ConsistentNormals to 1 if you want your frames to be 'smooth'.
    //! Note that in this case, the normal to the curve will not represent the
    //! acceleration, ie this is no more Frenet-Serret chart.
    vtkBooleanMacro(ConsistentNormals, int);
    vtkSetMacro(ConsistentNormals, int);
    vtkGetMacro(ConsistentNormals, int);

    //! If yes, computes the cross product between Tangent and Normal to get
    //! the binormal vector.
    vtkBooleanMacro(ComputeBinormal, int);
    vtkSetMacro(ComputeBinormal, int);
    vtkGetMacro(ComputeBinormal, int);

    //! Define the inclination of the consistent normals. Radian value.
    vtkSetMacro(ViewUp, double);
    vtkGetMacro(ViewUp, double);

    //! Rotate a vector around an axis
    //! \param [in] axis {Vector defining the axis to turn around.}
    //! \param [in] angle {Rotation angle in radian.}
    //! \param [out] vector {Vector to rotate. In place modification.}
    static void RotateVector(double* vector, const double* axis, double angle);


protected:
    CBCTFrenetSerretFrame();
    ~CBCTFrenetSerretFrame();

    virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
    virtual int FillInputPortInformation(int port, vtkInformation* info) override;

    //! Computes the derivative between 2 points (Next - Last).
    //! \param [in] pointIdNext {give first point Id}
    //! \param [in] pointIdLast {give second point Id}
    //! \param [out] tangent {fill a 3-array with the derivative value}
    //! \note If Next is [i+1], Last is [i-1], your are computing the
    //! centered tangent at [i].
    void ComputeTangentVectors(vtkIdType pointIdNext,
        vtkIdType pointIdLast,
        double* tangent);

    //! Computes the second derivative between 2 points (Next - Last).
    //! \param [in] nextTg {give a first derivative}
    //! \param [in] lastTg {give a first derivative}
    //! \param [out] normal {fill a 3-array with the second derivative value}
    void ComputeNormalVectors(double* tgNext,
        double* tgLast,
        double* normal);

    //! ConsistentNormal depends on the local tangent and the last computed
    //! normal. This is a projection of lastNormal on the plan defined
    //! by tangent.
    //! \param [in] tangent {give the tangent}
    //! \param [in] lastNormal {give the reference normal}
    //! \param [out] normal {fill a 3-array with the normal vector}
    void ComputeConsistentNormalVectors(double* tangent,
        double* lastNormal,
        double* normal);
private:
    CBCTFrenetSerretFrame(const CBCTFrenetSerretFrame&) = delete;
    void operator=(const CBCTFrenetSerretFrame&) = delete;

    int ComputeBinormal; //!< If 1, a Binormal array is added to the output
    int ConsistentNormals; //!< Boolean. If 1, successive normals are computed
    //!< in smooth manner.
    //!< \see ComputeConsistentNormalVectors
    double ViewUp; //!< Define the inclination of the normal vectors in case of
    //!< ConsistentNormals is On
};

#endif //__CBCTFrenetSerretFrame_H__