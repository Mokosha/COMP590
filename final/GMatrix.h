#ifndef GMATRIX_H_
#define GMATRIX_H_

#include "GVector.h"

#include <algorithm>

template<typename T, const int nRows, const int nCols>
class GMatrix {
 protected:
  static const int kNumElements = nRows * nCols;
  T mat[kNumElements];

 public:
  // Constructors
  GMatrix() { }
  GMatrix(const GMatrix<T, nRows, nCols> &other) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] = other[i];
    }
  }

  // Accessors
  T &operator()(int idx) { return mat[idx]; }
  const T &operator()(int idx) const { return mat[idx]; }
  T &operator()(int r, int c) { return mat[r * nCols + c]; }
  const T &operator()(int r, int c) const { return mat[r * nCols + c]; }

  T &operator[](int idx) { return mat[idx]; }
  const T &operator[](int idx) const { return mat[idx]; }

  GMatrix<T, nRows, nCols> &operator=(const GMatrix<T, nRows, nCols> &other) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] = other[i];
    }    
  }

  // Operators
  template<typename _T>
  GMatrix<T, nRows, nCols> operator+(
    const GMatrix<_T, nRows, nCols> &m
  ) const {
    GMatrix<T, nRows, nCols> a;
    for(int i = 0; i < kNumElements; i++) {
      a[i] = mat[i] + m[i];
    }
    return a;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> &operator+=(
    const GMatrix<_T, nRows, nCols> &m
  ) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] += m[i];
    }
    return *this;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> operator-(
    const GMatrix<_T, nRows, nCols> &m
  ) const {
    GMatrix<T, nRows, nCols> a;
    for(int i = 0; i < kNumElements; i++) {
      a[i] = mat[i] - m[i];
    }
    return a;
  }

  template<typename _T>
  GMatrix<T, nRows, nCols> &operator-=(
    const GMatrix<_T, nRows, nCols> &m
  ) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] -= m[i];
    }
    return *this;
  }

  GMatrix<T, nRows, nCols> operator*(T s) const {
    GMatrix<T, nRows, nCols> a;
    for(int i = 0; i < kNumElements; i++) {
      a[i] = mat[i] * s;
    }
    return a;
  }

  GMatrix<T, nRows, nCols> &operator*=(T s) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] *= s;
    }
    return *this;
  }

  GMatrix<T, nRows, nCols> operator/(T s) const {
    GMatrix<T, nRows, nCols> a;
    for(int i = 0; i < kNumElements; i++) {
      a[i] = mat[i] / s;
    }
    return a;
  }

  GMatrix<T, nRows, nCols> &operator/=(T s) {
    for(int i = 0; i < kNumElements; i++) {
      mat[i] /= s;
    }
    return *this;
  }

  // Matrix multiplication
  template<typename _T, const int nTarget>
  GMatrix<T, nRows, nTarget> operator*(
    const GMatrix<_T, nCols, nTarget> &m
  ) const {
    GMatrix<T, nRows, nTarget> result;
    for(int r = 0; r < nRows; r++)
    for(int c = 0; c < nTarget; c++) {
      result(r, c) = 0;
      for(int j = 0; j < nCols; j++) {
        result(r, c) += (*this)(r, j) * m(j, c);
      }
    }
    return result;
  }

  template<typename _T, const int nTarget>
  GMatrix<T, nRows, nTarget> &operator*=(
    const GMatrix<_T, nCols, nTarget> &m
  ) {
    (*this) = (*this) * m;
    return (*this);
  }

  // Vector multiplication
  template<typename _T>
  GVector<T, nCols> operator*(const GVector<_T, nCols> &v) const {
    GVector<T, nCols> result;
    for(int r = 0; r < nRows; r++) {
      result(r) = 0;
      for(int j = 0; j < nCols; j++) {
        result(r) += (*this)(r, j) * v(j);
      }
    }
    return result;
  }
};

template<typename T, unsigned N>
class GMatrixSquare : public GMatrix<T, N, N> {
 protected:
  template<typename _T>
  void CopyFrom(const GMatrix<_T, N, N> &other) {
    for(int i = 0; i < N*N; i++) {
      this->mat[i] = other[i];
    }
  }
 public:
  GMatrixSquare() : GMatrix<T, N, N>() {
    Identity();
  }
  template<typename _T>
  GMatrixSquare(const GMatrix<_T, N, N> &other) {
    this->CopyFrom(other);
  }

  void Identity() {
    for(int j = 0; j < N; j++) {
      for(int i = 0; i < N; i++) {
        if(i == j)
          (*this)(i, j) = static_cast<T>(1.0f);
        else
          (*this)(i, j) = static_cast<T>(0.0f);
      }
    }
  }
};

template<typename T>
class GMatrix2x2 : public GMatrixSquare<T, 3> {
 public:
  GMatrix2x2() : GMatrixSquare<T, 3>() { }

  template<typename _T>
  GMatrix2x2(const GMatrix<_T, 2, 2> &other) {
    this->CopyFrom(other);
  }

  float Determinant() const {
    const GMatrix2x2<T> &m = *this;
    return m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1);
  }

  bool Invert() {
    float determinant = Determinant();
    if(determinant == 0.0f)
      return false;

    GMatrix2x2<T> &m = *this;
    float d = 1.0f / determinant;
    std::swap(m(0, 0), m(1, 1));
    m(0, 1) = -m(0, 1);
    m(1, 0) = -m(1, 0);
    m *= d;
  }
};

template<typename T>
class GMatrix3x3 : public GMatrixSquare<T, 3> {
 public:
  GMatrix3x3() : GMatrixSquare<T, 3>() { }

  template<typename _T>
  GMatrix3x3(const GMatrix<_T, 3, 3> &other) {
    this->CopyFrom(other);
  }

  float Determinant() const {
    const GMatrix3x3<T> &m = *this;
    return 0.0f
      + m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2))
      - m(0, 1) * (m(1, 0) * m(2, 2) - m(2, 0) * m(0, 2))
      + m(0, 2) * (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1));
  }

  bool Invert() {
    float determinant = Determinant();
    if(determinant == 0.0f)
      return false;

    GMatrix3x3<T> &m = *this;
    float d = 1.0f / determinant;

    float m00 = m(2, 2) * m(1, 1) - m(2, 1) * m(1, 2);
    float m01 = m(0, 2) * m(2, 1) - m(2, 2) * m(0, 1);
    float m02 = m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2);
    float m10 = m(1, 2) * m(2, 0) - m(2, 2) * m(1, 0);
    float m11 = m(0, 0) * m(2, 2) - m(2, 0) * m(0, 2);
    float m12 = m(0, 2) * m(1, 0) - m(1, 2) * m(0, 0);
    float m20 = m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1);
    float m21 = m(0, 1) * m(2, 0) - m(2, 1) * m(0, 0);
    float m22 = m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1);
    m(0, 0) = m00;
    m(0, 1) = m01;
    m(0, 2) = m02;
    m(1, 0) = m10;
    m(1, 1) = m11;
    m(1, 2) = m12;
    m(2, 0) = m20;
    m(2, 1) = m21;
    m(2, 2) = m22;
    m *= d;
  }
};

typedef GMatrix2x2<float> GMatrix2x2f;
typedef GMatrix3x3<float> GMatrix3x3f;

#endif  // GMATRIX_H_
