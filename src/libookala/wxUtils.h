#ifndef WXUTILS_H_HAS_BEEN_INCLUDED
#define WXUTILS_H_HAS_BEEN_INCLUDED

inline wxString _U(const char String[] = "")
{
    return wxString(String, wxConvUTF8);
}

inline wxString _S(std::string String)
{
    return wxString(String.c_str(), wxConvUTF8);
}

#endif

