/*
MIT License

Copyright (c) 2017-2020 Markus Trenkwalder

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "ObjData.h"

#include <fstream>
#include <string>
#include <sstream>

#include <iostream>
#include <map>

using namespace std;

typedef vmath::vec3<float> vec3f;
typedef vmath::vec2<float> vec2f;

// string is supposed to be in the form <vertex_index>/<texture_index>/<normal_index>, 
void getIndicesFromString(string str, unsigned &vi, unsigned &ni, unsigned &ti)
{
	string v;
	string t;
	string n;

	string *tmp[3] = {&v, &t, &n};
	
	int tidx = 0;
	string *target = tmp[0];
	
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == '/') target = tmp[++tidx];
		else *target += str[i];
	}

	istringstream iss1(v);
	vi = 0;
	iss1 >> vi;

	istringstream iss2(t);
	ti = 0;
	iss2 >> ti;

	istringstream iss3(n);
	ni = 0;
	iss3 >> ni;
}

ObjData ObjData::loadFromFile(const char *filename)
{
	ObjData result;
	result.vertices.push_back(vec3f(0.0f));
	result.normals.push_back(vec3f(0.0f));
	result.texcoords.push_back(vec2f(0.0));

	ifstream in(filename, ios::in);
	while (in) {
		string line;
		getline(in, line);
		istringstream iss;
		iss.str(line);
		string cmd;
		iss >> cmd;
		
		if (cmd[0] == '#') continue;
		else if (cmd == "v") {
			vec3f v;
			iss >> v.x >> v.y >> v.z;
			result.vertices.push_back(v);
		} else if (cmd == "vn") {
			vec3f v;
			iss >> v.x >> v.y >> v.z;
			result.normals.push_back(v);
		} else if (cmd == "vt") {
			vec2f t;
			iss >> t.x >> t.y;
			result.texcoords.push_back(t);
		} else if (cmd == "f") {
			result.faces.push_back(ObjData::Face());
			ObjData::Face &face = result.faces.back();
			
			while (iss) {
				string word;
				iss >> word;

				if (word == "") continue;
				
				ObjData::VertexRef vr;
				getIndicesFromString(word, vr.vertexIndex, vr.normalIndex, vr.texcoordIndex);

				face.push_back(vr);
			}
		}
	}

	return result;
}

namespace internal {
	struct VertexRefCompare {
		bool operator () (const ObjData::VertexRef &v1, const ObjData::VertexRef &v2) const
		{
			if (v1.vertexIndex < v2.vertexIndex) return true;
			else if (v1.vertexIndex > v2.vertexIndex) return false;
			else {
				if (v1.normalIndex < v2.normalIndex) return true;
				else if (v1.normalIndex > v2.normalIndex) return false;
				else {
					if (v1.texcoordIndex < v2.texcoordIndex) return true;
					else return false;
				}
			}
		}
	};
}

using namespace internal;

void ObjData::toVertexArray(std::vector<VertexArrayData> &vdata, std::vector<int> &idata)
{
	vdata.clear();
	idata.clear();

	struct Helper {
		vector<VertexArrayData> &m_vdata;
		vector<int> &m_idata;
		const ObjData &m_obj;

		typedef map<VertexRef, unsigned, internal::VertexRefCompare> vertex_index_map_t;
		vertex_index_map_t vertex_index_map;

		Helper(const ObjData& obj, std::vector<VertexArrayData> &vdata, std::vector<int> &idata):
			m_vdata(vdata), m_idata(idata), m_obj(obj) {}

		void process() 
		{
			for (size_t i = 0; i < m_obj.faces.size(); ++i) {
				unsigned i1 = addVertex(m_obj.faces[i][0]);

				// make a triangle fan if there are more than 3 vertices
				for (size_t j = 2; j < m_obj.faces[i].size(); ++j) {
					unsigned i2 = addVertex(m_obj.faces[i][j-1]);
					unsigned i3 = addVertex(m_obj.faces[i][j]);

					addFace(i1, i2, i3);
				}
			}
		}

		unsigned addVertex(const VertexRef &v) 
		{
			// check if this vertex already exists
			vertex_index_map_t::const_iterator it = vertex_index_map.find(v);
			if (it != vertex_index_map.end()) 
				return it->second;

			// does not exist, so insert a new one
			unsigned i = static_cast<unsigned>(vertex_index_map.size());
			vertex_index_map.insert(make_pair(v, i));

			VertexArrayData vd;
			vd.vertex = m_obj.vertices[v.vertexIndex];
			vd.normal = m_obj.normals[v.normalIndex];
			vd.texcoord = m_obj.texcoords[v.texcoordIndex];
			m_vdata.push_back(vd);

			return i;
		}

		void addFace(int i1, int i2, int i3) 
		{
			m_idata.push_back(i1);
			m_idata.push_back(i2);
			m_idata.push_back(i3);
		}

	} helper(*this, vdata, idata);

	helper.process();
}
