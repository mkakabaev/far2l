#pragma once

/*
namelist.hpp

Список имен файлов, используется в viewer при нажатии Gray+/Gray-
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "DList.hpp"
#include "plugin.hpp"
#include "FARString.hpp"

class NamesList
{
	private:
		struct FileName2
		{
			FARString strName;
		};

		struct OneName
		{
			struct FileName2 Value;

			OneName()
			{
			}
			// для перекрывающихся объектов поведение как у xstrncpy!
			const OneName& operator=(struct FileName2 &rhs)
			{
				Value.strName = rhs.strName;
				return *this;
			}
		};

		typedef DList<OneName> StrList;

		StrList Names;
		const OneName *CurrentName;

		FARString strCurrentDir;

	private:
		void Init();

	public:
		NamesList();
		~NamesList();

	public:
		void AddName(const wchar_t *Name);
		bool GetNextName(FARString &strName);
		bool GetPrevName(FARString &strName);
		void SetCurName(const wchar_t *Name);
		void MoveData(NamesList &Dest);
		void GetCurDir(FARString &strDir);
		void SetCurDir(const wchar_t *Dir);
};
