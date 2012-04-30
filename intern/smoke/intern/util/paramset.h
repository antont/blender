/******************************************************************************
 * Parameter storage class
 *
 * Copyright(c) 1998-2007 Matt Pharr and Greg Humphreys.
 *
 *****************************************************************************/

#ifndef PBRT_PARAMSET_H
#define PBRT_PARAMSET_H

#include "globals.h"
using std::string;
using std::vector;
#if (_MSC_VER >= 1400) // NOBOOK
#include <stdio.h>     // NOBOOK
#define snprintf _snprintf // NOBOOK
#endif // NOBOOK

// forward declaration
template <class T> struct ParamSetItem;

// ParamSet Declarations
#if DDF_USEPARSER == 1
#include "parser.h"
class COREDLL ParamSet {
#else
class ParamSet {
#endif
public:
	// ParamSet Public Methods
	ParamSet() { }
	ParamSet &operator=(const ParamSet &p2);
	ParamSet(const ParamSet &p2);
	void AddSet(const ParamSet &p2);
	void AddFloat(const string &, const float *, int nItems = 1);
	void AddInt(const string &, const int *, int nItems = 1);
	void AddBool(const string &, const bool *, int nItems = 1);
	void AddVector(const string &, const DDF::Vec3 *, int nItems = 1);
	void AddString(const string &, const string *, int nItems = 1);
	
	void AddInt(const string &n, int d) { AddInt(n,&d); }
	void AddBool(const string &n, bool d)  { AddBool(n,&d); }
	void AddFloat(const string &n, float d)  { AddFloat(n,&d); }
	void AddVector(const string &n, const DDF::Vec3& d) { AddVector(n,&d); }
	void AddString(const string &n, const string& d) { AddString(n,&d); }

	bool EraseInt(const string &);
	bool EraseBool(const string &);
	bool EraseFloat(const string &);
	bool EraseVector(const string &);
	bool EraseString(const string &);

	float FindOneFloat(const string &, float d) const;
	int FindOneInt(const string &, int d) const;
	bool FindOneBool(const string &, bool d) const;
	DDF::Vec3 FindOneVector(const string &, const DDF::Vec3 &d) const;
	string FindOneString(const string &, const string &d) const;
	//string FindTexture(const string &) const;

	const float *FindFloat(const string &, int *nItems) const;
	const int *FindInt(const string &, int *nItems) const;
	const bool *FindBool(const string &, int *nItems) const;
	const DDF::Vec3 *FindVector(const string &, int *nItems) const;
	const string *FindString(const string &, int *nItems) const;

	void override(const ParamSet &p2);

	void ReportUnused() const;
	~ParamSet() {
		Clear();
	}
	void Clear();
	string ToString() const;
private:
	// ParamSet Data
	vector<ParamSetItem<int> *> ints;
	vector<ParamSetItem<bool> *> bools;
	vector<ParamSetItem<float> *> floats;
	vector<ParamSetItem<DDF::Vec3> *> vectors;
	vector<ParamSetItem<string> *> strings;
};

template <class T> struct ParamSetItem {
	// ParamSetItem Public Methods
	ParamSetItem<T> *Clone() const {
		return new ParamSetItem<T>(name, data, nItems);
	}
	ParamSetItem(const string &name, const T *val, int nItems = 1);
	~ParamSetItem() {
		delete[] data;
	}
	// ParamSetItem Data
	string name;
	int nItems;
	T *data;
	bool lookedUp;
};
// ParamSetItem Methods
template <class T>
ParamSetItem<T>::ParamSetItem(const string &n,
							  const T *v,
							  int ni) {
	name = n;
	nItems = ni;
	data = new T[nItems];
	for (int i = 0; i < nItems; ++i)
		data[i] = v[i];
	lookedUp = false;
}



#endif // PBRT_PARAMSET_H
