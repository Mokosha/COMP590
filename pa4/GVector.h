
#ifndef GVECTOR_H_
#define GVECTOR_H_

template <typename T, const int N>
class GVector {
 protected:

  // Vector representation
  T vec[N];

 public:
    
  GVector() { }
  GVector(const GVector<T, N> &other) {
    for(int i = 0; i < N; i++) vec[i] = other[i];
  }

  GVector(T *_vec) {
    for(int i = 0; i < N; i++) {
      vec[i] = _vec[i];
    }
  }

  // Accessors
  T &operator()(int idx) { return vec[idx]; }
  T &operator[](int idx) { return vec[idx]; }
  const T &operator()(int idx) const { return vec[idx]; }
  const T &operator[](int idx) const { return vec[idx]; }

  // Allow casts to the respective array representation...
  operator T *() const { return vec; }
  GVector<T, N> &operator=(const T *v) {
    for(int i = 0; i < N; i++) this->vec[i] = v[i];
    return *this;
  }

  // Allows casting to other vector types if the underlying type system does as well...
  template<typename _T>
  operator GVector<_T, N>() const { 
    return GVector<_T, N>(vec); 
  }

  // Operators
  template<typename _T>
  GVector<T, N> operator+(const GVector<_T, N> &v) const {
    GVector a;
    for(int i = 0; i < N; i++) {
      a.vec[i] = v(i) + this->vec[i];
    }
    return a;
  }

  template<typename _T>
  GVector<T, N> &operator+=(const GVector<_T, N> &v) {
    for(int i = 0; i < N; i++) {
      this->vec[i] += v(i);
    }
    return *this;
  }

  template<typename _T>
  GVector<T, N> operator-(const GVector<_T, N> &v) const {
    GVector<T, N> a;
    for(int i = 0; i < N; i++) {
      a(i) = this->vec[i] - v[i];
    }
    return a;
  }

  template<typename _T>
  GVector<T, N> &operator-=(const GVector<_T, N> &v) {
    for(int i = 0; i < N; i++) {
      this->vec[i] -= v[i];
    }
    return *this;
  }

  template<typename _T>
  GVector<T, N> &operator=(const GVector<_T, N> &v) {
    for(int i = 0; i < N; i++) {
      this->vec[i] = v[i];
    }
    return *this;
  }

  GVector<T, N> operator*(const T &s) const {
    GVector<T, N> a;
    for(int i = 0; i < N; i++) a[i] = vec[i] * s;
    return a;
  }
  
  friend GVector<T, N> operator*(const T &s, const GVector<T, N> &v) {
    GVector<T, N> a;
    for(int i = 0; i < N; i++)
      a[i] = v[i] * s;
    return a;
  }

  GVector<T, N> operator/(const T &s) const {
    GVector<T, N> a;
    for(int i = 0; i < N; i++) a[i] = vec[i] / s;
    return a;
  }
  
  friend GVector<T, N> operator/(const T &s, const GVector<T, N> &v) {
    GVector<T, N> a;
    for(int i = 0; i < N; i++) a[i] = v[i] / s;
    return a;
  }

  void operator*=(const T &s) {
    for(int i = 0; i < N; i++) vec[i] *= s;
  }

  void operator/=(const T &s) {
    for(int i = 0; i < N; i++) vec[i] /= s;
  }

  // Vector operations
  template<typename _T>
  T Dot(const GVector<_T, N> &v) const {
    T sum = 0;
    for(int i = 0; i < N; i++) sum += vec[i] * v[i];
    return sum;
  }

  T LengthSq() const { return this->Dot(*this); }
  T Length() const { return sqrt(LengthSq()); }
};

template<typename T>
class GVector2 : public GVector<T, 2> {
 private:
  template<typename _T>
  void CopyFrom(const GVector<_T, 2> &other) {
    X() = other[0];
    Y() = other[1];
  }
 public:
  GVector2(T x, T y) : GVector<T, 2>(){
    this->vec[0] = x;
    this->vec[1] = y;
  }

  template<typename _T>
  GVector2(const GVector<_T, 2> &other) { CopyFrom(other); }

  template<typename _T>
  GVector2<T> &operator=(const GVector<_T, 2> &other) {
    CopyFrom(other);
  }

  const T &X() const { return this->vec[0]; }
  T &X() { return this->vec[0]; }

  const T &Y() const { return this->vec[1]; }
  T &Y() { return this->vec[1]; }
};

template<typename T>
class GVector3 : public GVector<T, 3> {
 private:
  template<typename _T>
  void CopyFrom(const GVector<_T, 3> &other) {
    X() = other[0];
    Y() = other[1];
    Z() = other[2];
  }
 public:
  GVector3(T x, T y, T z) : GVector<T, 3>() {
    this->vec[0] = x;
    this->vec[1] = y;
    this->vec[2] = z;
  }

  template<typename _T>
  GVector3(const GVector<_T, 3> &other) { CopyFrom(other); }

  template<typename _T>
  GVector3<T> &operator=(const GVector<_T, 3> &other) {
    CopyFrom(other);
  }

  const T &X() const { return this->vec[0]; }
  T &X() { return this->vec[0]; }

  const T &Y() const { return this->vec[1]; }
  T &Y() { return this->vec[1]; }

  const T &Z() const { return this->vec[2]; }
  T &Z() { return this->vec[2]; }
};

typedef GVector2<float> GVec2f;
typedef GVector3<float> GVec3f;

#endif  // GVECTOR_H_
