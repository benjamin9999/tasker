#include "stdafx.h"
#include <iostream>

#include "material.h"

using namespace std;


void MaterialStore::print() const {
	cout << "----- material pack -----" << endl;
	for (int i = 0; i < store.size(); i++) {
		auto& item = store[i];
		cout << "id = " << i << ", ";
		item.print();
	}
	cout << "---  end of materials ---" << endl; }


int MaterialStore::find_by_name(const std::string& name) const {
	for (unsigned i = 0; i<store.size(); i++) {
		if (store[i].name == name)
			return i; }
	return 0; }


void Material::print() const {
	cout << "material[" << this->name << "]:" << endl;
	cout << "  ka" << this->ka << ", ";
	cout << "kd" << this->kd << ", ";
	cout << "ks" << this->ks << endl;
	cout << "  specpow(" << this->specpow << "), density(" << this->d << ")" << endl;
	cout << "  texture[" << this->imagename << "]" << endl; }