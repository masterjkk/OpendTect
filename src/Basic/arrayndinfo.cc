/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/

static const char* rcsID = "$Id: arrayndinfo.cc,v 1.7 2006-07-10 17:42:33 cvskris Exp $";

#include <arrayndinfo.h>

void ArrayNDInfo::getArrayPos( uint64 mempos, int* pos ) const
{
    int ndim = getNDim();
    for ( int idx=0; idx<ndim; idx++ )
    {
	int tsz = 1;
	for ( int idy=idx+1; idy<ndim; idy++ )
	    tsz *= getSize(idy);

	pos[idx] = mempos/tsz;
	mempos = mempos%tsz;
    }
}
	


Array1DInfoImpl::Array1DInfoImpl( int nsz ) 
	: sz( nsz ) 
{ 
    totalSz = sz; 
}


Array1DInfoImpl::Array1DInfoImpl( const Array1DInfo& nsz)
	: sz( nsz.getSize(0) )
{ 
    totalSz = sz;
} 


int Array1DInfoImpl::getSize( int dim ) const
{ 
    return dim ? 0 : sz;
}


bool Array1DInfoImpl::setSize( int dim, int nsz )
{
    if( dim != 0 ) return false;
    sz = nsz;
    totalSz = sz;	
    return true;
}


uint64 Array1DInfoImpl::getMemPos( const int* pos ) const
{
    return pos[0];
}


uint64 Array1DInfoImpl::getMemPos( int p ) const
{
    return p;
}


bool Array1DInfoImpl::validPos( const int* pos ) const
{
    int idx = pos[0];
    return idx < 0 || idx >= sz ? false : true;
}


Array2DInfoImpl::Array2DInfoImpl( int sz0, int sz1 )
{
    sz[0] = sz0; sz[1] = sz1;
    totalSz = calcTotalSz();
}


Array2DInfoImpl::Array2DInfoImpl( const Array2DInfo& nsz)
{
    sz[0] = nsz.getSize(0);
    sz[1] = nsz.getSize(1);

    totalSz = calcTotalSz(); 
}


int Array2DInfoImpl::getSize( int dim ) const
{
    return dim > 1 || dim < 0 ? 0 : sz[dim];
}


bool Array2DInfoImpl::setSize( int dim, int nsz )
{
    if( dim > 1 || dim < 0 ) return false;
    sz[dim] = nsz;
    totalSz = calcTotalSz();	
    return true;
}


uint64 Array2DInfoImpl::getMemPos( const int* pos ) const
{
    return (uint64) pos[0] * sz[1] + pos[1];
}


uint64 Array2DInfoImpl::getMemPos( int p0, int p1 ) const
{
    return (uint64) p0 * sz[1] + p1;
}


bool Array2DInfoImpl::validPos( const int* pos ) const
{
    for ( int idx = 0; idx < 2; idx++ )
    {
        int p = pos[idx];
        if( p < 0 || p >= sz[idx] ) return false;
    }
    return true;
}

bool Array2DInfoImpl::validPos( int p0, int p1 ) const
{ 
    return p0 < 0 || p0 >= sz[0] || p1 < 0 || p1 >= sz[1] ? false : true;
}



uint64 Array2DInfoImpl::calcTotalSz() const
{
    return (uint64) sz[0] * sz[1];
}


Array3DInfoImpl::Array3DInfoImpl( int sz0, int sz1, int sz2) 
{ 
    sz[0] = sz0; sz[1] = sz1; sz[2] = sz2;
    totalSz = calcTotalSz();
}


Array3DInfoImpl::Array3DInfoImpl( const Array3DInfo& nsz)
{
    sz[0] = nsz.getSize(0);
    sz[1] = nsz.getSize(1);
    sz[2] = nsz.getSize(2);

    totalSz = calcTotalSz();
}


int Array3DInfoImpl::getSize(int dim) const
{
    return dim > 2 || dim < 0 ? 0 : sz[dim];
}


bool Array3DInfoImpl::setSize(int dim, int nsz)
{
    if( dim > 2 || dim < 0 ) return false;
    sz[dim] = nsz;
    totalSz = calcTotalSz(); 
    return true;
}


uint64 Array3DInfoImpl::getMemPos(const int* pos) const
{
    return (uint64) pos[0] * sz[2] * sz[1] + pos[1] * sz[2] + pos[2]; 
}


uint64 Array3DInfoImpl::getMemPos(int p0, int p1, int p2) const
{
    return (uint64) p0 * sz[2] * sz[1] + p1 * sz[2] + p2;
}



bool Array3DInfoImpl::validPos(const int* pos) const
{
    for ( int idx = 0; idx < 3; idx++ )
    {	
	int p = pos[idx];
	if( p < 0 || p >= sz[idx] ) return false;
    }
    return true; 
}


bool Array3DInfoImpl::validPos( int p0, int p1, int p2 ) const
{
    return p0 < 0 || p0 >= sz[0]
	|| p1 < 0 || p1 >= sz[1]
	|| p2 < 0 || p2 >= sz[2] ? false : true;

    return true;
}


uint64 Array3DInfoImpl::calcTotalSz() const
{
    return (uint64) sz[0] * sz[1] * sz[2];
}	 


ArrayNDInfo* ArrayNDInfoImpl::clone() const
{
    if ( ndim==1 ) return new Array1DInfoImpl(sizes[0]);
    if ( ndim==2 ) return new Array2DInfoImpl(sizes[0], sizes[1]);
    if ( ndim==3 ) return new Array3DInfoImpl(sizes[0], sizes[1], sizes[2]);

    return new ArrayNDInfoImpl(*this); 
}


ArrayNDInfo* ArrayNDInfoImpl::create( int ndim )
{
    if ( ndim==1 ) return new Array1DInfoImpl;
    if ( ndim==2 ) return new Array2DInfoImpl;
    if ( ndim==3 ) return new Array3DInfoImpl;

    return new ArrayNDInfoImpl(ndim); 
}


ArrayNDInfoImpl::ArrayNDInfoImpl( int ndim_ )
	: sizes( new int[ndim_] )  
	, ndim( ndim_ )
{
    totalSz = 0;
    for ( int idx = 0; idx < ndim; idx++ )
	sizes[idx] = 0;
}


ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfoImpl& nsz )
	: sizes(new int[nsz.getNDim()]) 
	, ndim( nsz.getNDim() )
{
    for (int idx = 0; idx < ndim; idx++)
	setSize( idx, nsz.getSize(idx) ); 
}


ArrayNDInfoImpl::ArrayNDInfoImpl( const ArrayNDInfo& nsz )
	: sizes(new int[nsz.getNDim()]) 
	, ndim( nsz.getNDim() )
{
    for (int idx = 0; idx < ndim; idx++)
	setSize( idx, nsz.getSize(idx) ); 
}

ArrayNDInfoImpl::~ArrayNDInfoImpl()
{
    delete [] sizes;
}


int ArrayNDInfoImpl::getNDim() const
{
    return ndim;
}


bool ArrayNDInfoImpl::setSize( int dim, int newSz )
{
    if ( dim >= ndim || dim < 0 ) return false;

    sizes[dim] = newSz;
    totalSz = calcTotalSz();
    return true;
}


int ArrayNDInfoImpl::getSize( int dim ) const
{
    return dim >= ndim || dim < 0 ? 0 : sizes[dim];
}


uint64 ArrayNDInfoImpl::getMemPos( const int* pos ) const
{
    uint64 valueNr = 0;
    uint64 multiplicator = 1;

    for ( int idx = ndim-1; idx > -1; idx-- )
    {
	valueNr += pos[idx] * multiplicator;
        multiplicator *= sizes[idx];
    }

    return valueNr;
}


bool ArrayNDInfoImpl::validPos( const int* pos ) const
{
    int NDim = getNDim();
    
    for ( int idx=0; idx < NDim; idx++ )
    	if ( pos[idx] < 0 || pos[idx] >= sizes[idx] )
	    return false;
    return true;
}


uint64 ArrayNDInfoImpl::calcTotalSz() const
{ 
    int NDim = getNDim();
    uint64 size = 1;
 
    for ( int idx = 0; idx < NDim; idx++ )
        size *= sizes[idx]; 
    
    return size;
}


ArrayNDIter::ArrayNDIter( const ArrayNDInfo& sz )
    : sz_ ( sz )
    , position_( new int[sz.getNDim()] )
{
    reset();
}


ArrayNDIter::~ArrayNDIter()
{
    delete [] position_;
}


bool ArrayNDIter::next()
{
    return inc( sz_.getNDim() - 1 );
}


void ArrayNDIter::reset()
{
    const int ndim = sz_.getNDim();

    for ( int idx=0; idx<ndim; idx++ )
	position_[idx] = 0;
}


int ArrayNDIter::operator[](int dim) const
{
    return position_[dim];
}


bool ArrayNDIter::inc( int dim )
{
    position_[dim] ++;

    if ( position_[dim] >= sz_.getSize(dim))
    {
	if ( dim )
	{
	    position_[dim] = 0;
	    return inc( dim-1 );
	}
	else
	    return false;
    }

    return true;
}
