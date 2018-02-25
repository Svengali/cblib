#pragma once

class XMLReader;
class XMLWriter;

START_CB

/*
#define REFLECT_BEGIN( _CLASS, _PARENT ) \
virtual void Reflection( XMLReader &functor ) { Reflection<XMLReader>( functor ); } \
virtual void Reflection( XMLWriter &functor ) { Reflection<XMLWriter>( functor ); } \
template <class T> void Reflection(T & functor) { _PARENT::Reflection( functor );

#define REFLECT_BEGIN_ROOT( _CLASS ) \
virtual void Reflection( XMLReader &functor ) { Reflection<XMLReader>( functor ); } \
virtual void Reflection( XMLWriter &functor ) { Reflection<XMLWriter>( functor ); } \
template <class T> void Reflection(T & functor) {

#define REFLECT_BEGIN_CBLIB( _CLASS ) \
template <class T> void Reflection(T & functor) {

#define REFLECT(x)	functor(#x,x)

#define REFLECT_ARRAYN(what,count)	do{ for(int i=0;i<(count);i++) { char str[80]; sprintf(str,"%s%d",#what,i); functor(str,(what)[i]); } }while(0)

#define REFLECT_ARRAY(what)	REFLECT_ARRAYN(what,ARRAY_SIZE(what))

#define REFLECT_END(  ) }
*/

END_CB
