#pragma once
#include <atlbase.h>
#include <Shlwapi.h>
#include <string>
#include <vector>
#include <AppxPackaging.h>
#include "filedir.h"
#include "dynarr.h"
#include "norstr.h"
#include "version.h"
#include "raii.h"

HRESULT GetPackageReader (_In_ LPCWSTR inputFileName, _Outptr_ IAppxPackageReader **reader)
{
	HRESULT hr = S_OK;
	IAppxFactory* appxFactory = NULL;
	IStream* inputStream = NULL;
	hr = CoCreateInstance (__uuidof(AppxFactory), NULL, CLSCTX_INPROC_SERVER, __uuidof(IAppxFactory), (LPVOID*)(&appxFactory));
	if (SUCCEEDED (hr))
	{
		hr = SHCreateStreamOnFileEx (inputFileName, STGM_READ | STGM_SHARE_DENY_NONE, 0, FALSE, NULL, &inputStream);
	}
	if (SUCCEEDED (hr)) hr = appxFactory->CreatePackageReader (inputStream, reader);
	if (inputStream != NULL)
	{
		inputStream->Release ();
		inputStream = NULL;
	}
	if (appxFactory != NULL)
	{
		appxFactory->Release ();
		appxFactory = NULL;
	}
	return hr;
}
IAppxManifestReader *GetAppxManifestReader (IAppxPackageReader *reader)
{
	if (!reader) return nullptr;
	IAppxManifestReader *m = nullptr;
	if (FAILED (reader->GetManifest (&m)))
	{
		if (m) m->Release ();
		return nullptr;
	}
	return m;
}
IAppxManifestPackageId *GetAppxIdentity (IAppxManifestReader *m)
{
	if (!m) return nullptr;
	IAppxManifestPackageId *id = nullptr;
	if (FAILED (m->GetPackageId (&id)))
	{
		if (id) id->Release ();
		return nullptr;
	}
	return id;
}
IAppxManifestReader2 *GetAppxManifestReader2 (IAppxManifestReader *m)
{
	if (!m) return nullptr;
	IAppxManifestReader2 *m2 = nullptr;
	HRESULT hr = m->QueryInterface (__uuidof (IAppxManifestReader2), (void **)&m2);
	if (FAILED (hr)) 
	{
		if (m2) m2->Release ();
		return nullptr;
	}
	return m2;
}
HRESULT GetBundleReader (_In_ LPCWSTR inputFileName, _Outptr_ IAppxBundleReader** bundleReader)
{
	HRESULT hr = S_OK;
	IAppxBundleFactory* appxBundleFactory = NULL;
	IStream* inputStream = NULL;
	hr = CoCreateInstance (__uuidof(AppxBundleFactory), NULL, CLSCTX_INPROC_SERVER, __uuidof(IAppxBundleFactory), (LPVOID*)(&appxBundleFactory));
	if (SUCCEEDED (hr))
	{
		hr = SHCreateStreamOnFileEx (inputFileName, STGM_READ | STGM_SHARE_DENY_NONE, 0, FALSE, NULL, &inputStream);
	}
	if (SUCCEEDED (hr))
	{
		hr = appxBundleFactory->CreateBundleReader (inputStream, bundleReader);
	}
	if (inputStream != NULL)
	{
		inputStream->Release ();
		inputStream = NULL;
	}
	if (appxBundleFactory != NULL)
	{
		appxBundleFactory->Release ();
		appxBundleFactory = NULL;
	}
	return hr;
}
enum class PackageType
{
	resource, application
};

struct package_info
{
	struct identity
	{
		std::wstring name = L"";
		std::wstring publisher = L"";
		version version = 0;
		APPX_PACKAGE_ARCHITECTURE architecture = (APPX_PACKAGE_ARCHITECTURE)0;
		std::wstring package_family_name = L"";
		std::wstring package_full_name = L"";
		void init (IAppxManifestPackageId *id)
		{
			if (!id) return;
			CComPtr <IAppxManifestPackageId> pid = id;
			LPWSTR lpstr [4] = {nullptr};
			raii endtask ([&lpstr] () -> void {
				for (size_t cnt = 0; cnt < 4; cnt ++)
				{
					if (lpstr [cnt]) CoTaskMemFree (lpstr [cnt]);
					lpstr [cnt] = nullptr;
				}
			});
			if (FAILED (id->GetName (lpstr))) { if (lpstr [0]) { CoTaskMemFree (lpstr [0]); lpstr [0] = nullptr; } }
			if (FAILED (id->GetPublisher (lpstr + 1))) { if (lpstr [1]) { CoTaskMemFree (lpstr [1]); lpstr [1] = nullptr; } }
			if (FAILED (id->GetPackageFamilyName (lpstr + 2))) { if (lpstr [2]) { CoTaskMemFree (lpstr [2]); lpstr [2] = nullptr; } }
			if (FAILED (id->GetPackageFullName (lpstr + 3))) { if (lpstr [3]) { CoTaskMemFree (lpstr [3]); lpstr [3] = nullptr; } }
			if (lpstr [0]) name += lpstr [0];
			if (lpstr [1]) publisher += lpstr [1];
			if (lpstr [2]) package_family_name += lpstr [2];
			if (lpstr [3]) package_full_name += lpstr [3];
			UINT64 ver = 0;
			if (FAILED (id->GetVersion (&ver))) { ver = 0; }
			version.data (ver);
			id->GetArchitecture (&architecture);
		}
		identity (IAppxManifestPackageId *id = nullptr) { init (id); }
	} identity;
	bool is_valid = false;
	PackageType package_type = PackageType::resource;
	void init (IAppxPackageReader *r)
	{
		is_valid = false;
		if (!r) return;
		CComPtr <IAppxManifestReader> m = nullptr;
		if (SUCCEEDED (r->GetManifest (&m)) && m)
		{
			{
				CComPtr <IAppxManifestPackageId> id = nullptr;
				if (SUCCEEDED (m->GetPackageId (&id)) && id)
				{
					this->identity.init (id);
				}
			}
			CComPtr <IAppxManifestApplicationsEnumerator> a = nullptr;
			BOOL hasCurrent = false;
			if (SUCCEEDED (m->GetApplications (&a)) && a && SUCCEEDED (a->GetHasCurrent (&hasCurrent)) && hasCurrent) package_type = PackageType::application;
			else package_type = PackageType::resource;
			is_valid = true;
		}
	}
	void init (const std::wstring &fp)
	{
		is_valid = false;
		if (!IsFileExists (fp)) return;
		CComPtr <IAppxPackageReader> p = nullptr;
		if (SUCCEEDED (GetPackageReader (fp.c_str (), &p)) && p)
		{
			init (p);
		}
		else
		{
			is_valid = false;
		}
	}
	package_info (IAppxPackageReader *r) { init (r); }
	package_info (const std::wstring &fp) { init (fp); }
	package_info () {}
};

std::wstring GetBundlePackageFullName (IAppxBundleReader *r)
{
	if (!r) return L"";
	CComPtr <IAppxBundleManifestReader> m = nullptr;
	if (SUCCEEDED (r->GetManifest (&m)) && m)
	{
		CComPtr <IAppxManifestPackageId> id = nullptr;
		if (SUCCEEDED (m->GetPackageId (&id)) && id)
		{
			LPWSTR lpstr = nullptr;
			raii endtask ([&lpstr] () {
				if (lpstr) CoTaskMemFree (lpstr);
				lpstr = nullptr;
			});
			if (SUCCEEDED (id->GetPackageFullName (&lpstr)) && lpstr && !std::IsNormalizeStringEmpty (std::wstring (lpstr)))
			{
				return std::wstring (lpstr);
			}
		}
	}
	return L"";
}

std::wstring GetBundlePackageFullName (const std::wstring &filepath)
{
	CComPtr <IAppxBundleReader> bread = nullptr;
	if (SUCCEEDED (GetBundleReader (filepath.c_str (), &bread)) && bread)
	{
		return GetBundlePackageFullName (bread);
	}
	return L"";
}

std::wstring GetBundlePackagePublisher (IAppxBundleReader *r)
{
	if (!r) return L"";
	CComPtr <IAppxBundleManifestReader> m = nullptr;
	if (SUCCEEDED (r->GetManifest (&m)) && m)
	{
		CComPtr <IAppxManifestPackageId> id = nullptr;
		if (SUCCEEDED (m->GetPackageId (&id)) && id)
		{
			LPWSTR lpstr = nullptr;
			raii endtask ([&lpstr] () {
				if (lpstr) CoTaskMemFree (lpstr);
				lpstr = nullptr;
			});
			if (SUCCEEDED (id->GetPublisher (&lpstr)) && lpstr && !std::IsNormalizeStringEmpty (std::wstring (lpstr)))
			{
				return std::wstring (lpstr);
			}
		}
	}
	return L"";
}

std::wstring GetBundlePackagePublisher (const std::wstring &filepath)
{
	CComPtr <IAppxBundleReader> bread = nullptr;
	if (SUCCEEDED (GetBundleReader (filepath.c_str (), &bread)) && bread)
	{
		return GetBundlePackagePublisher (bread);
	}
	return L"";
}