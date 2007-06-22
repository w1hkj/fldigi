// data structures, includes dynamically allocated vectors and arrays
// (c) 1999-2003, Pawel Jalocha

#ifndef __STRUC_H__
#define __STRUC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========================================================================
// array re(sizing), copy, move, preset, etc.

template <class type>
 inline int ReallocArray(type **Array, size_t Size)
  {
// CINT doesn't accept NULL pointers to realloc
#ifdef __CINT__
//    if(*Array) free(*Array);
//    (*Array)=(type *)malloc(Size*sizeof(type));
    if(*Array) (*Array)=(type *)realloc(*Array,Size*sizeof(type));
          else (*Array)=(type *)malloc(Size*sizeof(type));
#else
    (*Array)=(type *)realloc(*Array,Size*sizeof(type));
#endif
    return (*Array)==0 ? -1:0; }

template <class type>
 int ReallocArraySafe(type **Array, size_t OldSize, size_t NewSize)
  { type *New; size_t Size;
    New=(type *)malloc(NewSize*sizeof(type)); if(New==0) return -1;
    Size=OldSize; if(NewSize<OldSize) Size=NewSize;
    if(Size) memcpy(New,*Array,Size*sizeof(type));
    free(*Array); (*Array)=New; return 0; }

template <class type>
 inline int AllocArray(type **Array, size_t Size)
  { (*Array)=(type *)malloc(Size*sizeof(type));
    return (*Array)==0; }

template <class type>
 inline void ClearArray(type *Array, size_t Size)
  { memset(Array,0,Size*sizeof(type)); }

template <class type>
 void PresetArray(type *Array, size_t Size, const type &Val)
  { size_t i; for(i=0; i<Size; i++) Array[i]=Val; }

template <class type>
 inline void CopyArray(type *Dst, type *Src, size_t Size)
  { memcpy(Dst,Src,Size*sizeof(type)); }

template <class type>
 inline void MoveArray(type *Dst, type *Src, size_t Size)
  { memmove(Dst,Src,Size*sizeof(type)); }

template <class type>
 void PrintArray(type *Array, size_t Size,
                 char *ValueFormat, char *IndexFormat="%3d", size_t Columns=10)
{ size_t Idx,Col;
  for(Col=0,Idx=0; Idx<Size; Idx++)
  { if(Col==0)
    { printf(IndexFormat,Idx); printf(":"); }
    printf(" ");
    printf(ValueFormat,Array[Idx]);
    Col++; if(Col>=Columns) { printf("\n"); Col=0; }
  }
  if(Col>0) printf("\n");
}

// -----------------------------------------------------------------------
// 2-dimensional array (re)sizing

template <class type> int AllocArray2D(type ***Array, size_t Rows, size_t Cols)
{ size_t i;
  (*Array)=(type **)malloc(Rows*(sizeof(type *)));
  if((*Array)==0) return -1;
  for(i=0; i<Rows; i++) (*Array)[i]=0;
  for(i=0; i<Rows; i++)
  { (*Array)[i]=(type *)malloc(Cols*sizeof(type));
    if((*Array)[i]==0) goto Error; }
  return 0;
Error:
  for(i=0; i<Rows; i++) free((*Array)[i]); free(*Array); (*Array)=0; return -1;
}
/*
template <class type> int AddRowsToArray2D(type ***Array, size_t Rows, size_t Cols,
                                           size_t NewRows=1)
{ size_t i;
  type **NewArray;
  NewArray=(type **)malloc((Rows+NewRows)*(sizeof(type *)));
  if(NewArray==0) return -1;
  memcpy(NewArray,*Array,Rows*sizeof(type *));
  return 0;
}
*/
template <class type> void FreeArray2D(type **Array, size_t Rows)
{ size_t i; for(i=0; i<Rows; i++) free(Array[i]); free(Array); }

template <class type> void ClearArray2D(type **Array, size_t Rows, size_t Cols)
{ size_t i; for(i=0; i<Rows; i++) ClearArray(Array[i],Cols); }

template <class type> void PresetArray2D(type **Array, size_t Rows, size_t Cols, type &Val)
{ size_t i; for(i=0; i<Rows; i++) PresetArray(Array[i],Cols,Val); }

// ========================================================================
// a sequence of any (fixed size) items

template <class type> class Seq
{ public:
   Seq(); ~Seq();          // default constructor/destructor
   Seq(Seq<type> &SrcSeq); // copy constructor
   void Init();            // user callable "constructor"
   int EnsureSpace(size_t ReqSpace); // make sure that there is enough space
   int EnsureSpace(void) { return EnsureSpace(Len); }
   int SetLen(size_t NewLen) { Len=NewLen; return EnsureSpace(Len); }
   int ReallocLen(size_t NewLen);
   int EnsureSpaceSafe(size_t ReqSpace);
   void Free(void);            // free all allocated space, remove all data
   void Clear(void) { Len=0; } // clear the sequence (set length to zero)
   size_t Index(type *Item) { return (size_t)(Item-Elem); }
   type &operator[] (int idx) { return (Elem[idx]); }   // [index] operator
   Seq<type> const &operator=(Seq<type> const &SrcSeq); // assignment operator
   type *First(void) { return Elem; }
   type *Last(void) { return Elem+(Len-1); }
   type *Append(size_t Num=1); // add one or more elements at the seq. end, returns the pointer to the new element(s)
   type *Insert(size_t Pos=0, size_t Num=1); // insert Num elements at Pos
   void Delete(size_t Pos=0, size_t Num=1); // delete Num elements at Pos
   void Shift(int Pos, const type &Fill); // shift left or right with filling
   int Truncate(size_t Margin=0); // remove unused but allocated space
   int Copy(type *Data, size_t DataLen);
   int Copy(Seq<type> &Src, size_t StartPos=0)  // copy the contains of another sequence
   { return Copy(Src.Elem+StartPos,Src.Len-StartPos); }
   int Copy(Seq<type> &Seq, size_t StartPos, size_t MaxLen);
   void Move(Seq<type> &Seq); // move the contains of another sequence, clear the other sequence
   int Join(const type &Data);
   int Join(type *Data, size_t DataLen); // join (append) given data
   int Join(Seq<type> &Seq) { return Join(Seq.Elem,Seq.Len); } // join a copy of the other sequence
   int NullTerm(void); // null-terminate (for character strings)
   void Reverse(void); // reverse the sequence order
   void FillWith(const type &Data, size_t StartPos=0); // fill with given value
   void Print(char *Format, size_t Columns=10);
   int SaveToFile(FILE *File);
   int LoadFromFile(FILE *File);
  public:
   size_t Space; // that many elements are allocated in *Elem
   size_t Len;	// that many elements are used up
   type *Elem;  // the storage space, contains Len elements
} ;

template <class type> Seq<type>::Seq() { Elem=0; Len=Space=0; }

template <class type> void Seq<type>::Init() { Elem=0; Len=Space=0; }

template <class type> Seq<type>::~Seq() { free(Elem); }

template <class type> Seq<type>::Seq(Seq<type> &SrcSeq)
{ if(EnsureSpace(SrcSeq.Len)==0)
  { Len=SrcSeq.Len; memcpy(Elem,SrcSeq.Elem,Len*sizeof(type)); }
}

template <class type> void Seq<type>::Free(void)
{ free(Elem); Elem=0; Space=Len=0; }

template <class type> int Seq<type>::EnsureSpace(size_t ReqSpace)
{ if(ReqSpace<=Space) return 0;
// CINT doesn't accept NULL pointers to realloc()
#ifdef __CINT__
  if(Elem) Elem=(type *)realloc(Elem,ReqSpace*sizeof(type));
      else Elem=(type *)malloc(ReqSpace*sizeof(type));
#else
  Elem=(type *)realloc(Elem,ReqSpace*sizeof(type));
#endif
  if(Elem==0) { Space=Len=0; return -1; }
  Space=ReqSpace; return 0; }

template <class type>
 Seq<type> const &Seq<type>::operator=(Seq<type> const &SrcSeq)
{ if((this!=&SrcSeq)&&(EnsureSpace(SrcSeq.Len)==0))
  { Len=SrcSeq.Len; memcpy(Elem,SrcSeq.Elem,Len*sizeof(type)); }
  return (*this); }

template <class type> int Seq<type>::ReallocLen(size_t NewLen)
{ Elem=(type *)realloc(Elem,NewLen*sizeof(type));
  if(Elem==0) { Space=Len=0; return -1; }
  Space=Len=NewLen; return 0; }

template <class type> int Seq<type>::Truncate(size_t Margin)
{ if(Space==(Len+Margin)) return 0;
  Elem=(type *)realloc(Elem,(Len+Margin)*sizeof(type));
  if(Elem==0) { Space=Len=0; return -1; }
  Space=Len+Margin; return 0; }

template <class type> void Seq<type>::Move(Seq<type> &Seq)
{ free(Elem);
  Len=Seq.Len; Space=Seq.Space; Elem=Seq.Elem;
  Seq.Elem=0; Seq.Len=Seq.Space=0; }

template <class type> int Seq<type>::Copy(type *Data, size_t DataLen)
{ if(EnsureSpace(DataLen)) return -1;
  memcpy(Elem,Data,DataLen*sizeof(type)); Len=DataLen; return 0; }

template <class type> int Seq<type>::Copy(Seq<type> &Src, size_t StartPos, size_t MaxLen)
{ int DataLen; DataLen=Src.Len-StartPos; if(DataLen>MaxLen) DataLen=MaxLen;
  if(EnsureSpace(DataLen)) return -1;
  memcpy(Elem,Src.Elem+StartPos,DataLen*sizeof(type)); Len=DataLen; return 0; }

template <class type> int Seq<type>::Join(const type &Data)
{ if(EnsureSpace(Len+1)) return -1;
  Elem[Len++]=Data; return 0; }

template <class type> int Seq<type>::Join(type *Data, size_t DataLen)
{ if(EnsureSpace(Len+DataLen)) return -1;
  memcpy(Elem+Len,Data,DataLen*sizeof(type)); Len+=DataLen; return 0; }

template <class type> type *Seq<type>::Insert(size_t Pos, size_t Num)
{ if(EnsureSpace(Len+Num)) return 0;
  // int p; for(p=Len-1; p>=Pos; p--) Elem[p+Num]=Elem[p];
  if((Len-Pos)>0) memmove(Elem+Pos+Num,Elem+Pos,(Len-Pos)*sizeof(type));
  Len+=Num; return Elem+Pos; }

template <class type> void Seq<type>::Delete(size_t Pos, size_t Num)
{ if((Len-Pos-Num)>0) memmove(Elem+Pos,Elem+Pos+Num,(Len-Pos-Num)*sizeof(type));
  Len-=Num; }

template <class type> type *Seq<type>::Append(size_t Num)
{ if(EnsureSpace(Len+Num)) return 0;
  Len+=Num; return Elem+(Len-Num); }
/*
template <class type> int Seq<type>::NullTerm(void)
{ if(EnsureSpace(Len+1)) return -1;
  Elem[Len]=0; return 0; }

int Seq<char>::NullTerm(void)
{ if(EnsureSpace(Len+1)) return -1;
  Elem[Len]=0; return 0; }
*/
template <class type> void Seq<type>::Reverse(void)
{ size_t i,j; type T;
  for(i=0,j=Len-1; i<j; i++,j--)
  { T=Elem[i]; Elem[i]=Elem[j]; Elem[j]=T; }
}

template <class type> void Seq<type>::FillWith(const type &Data, size_t StartPos)
{ size_t i; for(i=StartPos; i<Len; i++) Elem[i]=Data; }

template <class type> void Seq<type>::Shift(int Pos, const type &Fill)
{ size_t i;
  if(Pos>0) // shift toward higher positions - data inserted at the start of the sequence
  { if(Pos<Len) memmove(Elem+Pos,Elem,(Len-Pos)*sizeof(type)); else Pos=Len;
    for(i=0; i<Pos; i++) Elem[i]=Fill; }
  else if(Pos<0) // shift toward lower positions - data inserted at the end
  { Pos=(-Pos);
    if(Pos<Len) memmove(Elem,Elem+Pos,(Len-Pos)*sizeof(type)); else Pos=Len;
    for(i=Len-Pos; i<Len; i++) Elem[i]=Fill; }
}

template <class type> void Seq<type>::Print(char *Format, size_t Columns)
{ size_t i,Col;
  for(Col=0, i=0; i<Len; i++)
  { if(Col==0) { printf("%3d:",i); }
    printf(" "); printf(Format,Elem[i]); Col++;
    if(Col>=Columns) { printf("\n"); Col=0; } }
  if(Col>0) printf("\n");
}

// ========================================================================
// A 2-dimensional sequence

template <class type> class Seq2d
{ public:
   Seq2d(); ~Seq2d();
   int EnsureSpace(size_t rows, size_t cols); // make sure that there is enough space
   void Free(void);                     // free all space
   void Clear(void) { Rows=0; Cols=0; } // clear the sequence (set length to zero)
   // type &operator[] (int idx) { return (Elem[idx]); } // [idx] operator
   // type *Add(int Num=1); // add one or more elements, returns the index to the new element(s)
   size_t Rows,Cols;	    // that much is filled up
   size_t AllocRows,AllocCols; // that much is allocated
   type **Elem;  // contains Len elements
} ;

template <class type> Seq2d<type>::Seq2d()
{ Elem=0; AllocRows=Rows=AllocCols=Cols=0; }

template <class type> Seq2d<type>::~Seq2d()
{ size_t r; for(r=0; r<AllocRows; r++) free(Elem[r]); free(Elem); }

template <class type> void Seq2d<type>::Free(void)
{ size_t r; for(r=0; r<AllocRows; r++) free(Elem[r]); free(Elem);
  Elem=0; AllocRows=Rows=AllocCols=Cols=0; }

template <class type> int Seq2d<type>::EnsureSpace(size_t rows, size_t cols)
{ size_t r; type **New;
  if(rows>AllocRows)
  { New=(type**)malloc(rows*sizeof(type*));
    if(New==0) { Free(); return -1; }
    // printf("[Seq2d::EnsureSpace] AllocRows %d->%d\n",AllocRows,rows);
    if(Elem) { memcpy(New,Elem,AllocRows*sizeof(type**)); free(Elem); }
    Elem=New;
    for(r=AllocRows; r<rows; r++)
    { if(AllocCols)
      { Elem[r]=(type*)malloc(AllocCols*sizeof(type));
	if(Elem[r]==0) { Free(); return -1; }
      } else Elem[r]=0;
      AllocRows++; }
  }
  if(cols>AllocCols)
  { for(r=0; r<AllocRows; r++)
    { Elem[r]=(type*)realloc(Elem[r],cols*sizeof(type));
      if(Elem[r]==0) { Free(); return -1; } }
    // printf("[Seq2d::EnsureSpace] AllocCols %d->%d\n",AllocCols,cols);
    AllocCols=cols; }
  return 0; }


// ========================================================================

// A buffer structure for processing a continues series of samples.
// It is a linear array:
// - data between "Read" and "Write" is ready to be readout
// - data between "Write" and "Size" is still beeing written or processed
// - data before "Read" has been read-out and is thus obsolete
// As more data comes the samples must be shifted and the obsolete data removed:
// this is done by: RemovePastData()
   
template <class Type>
 class SampleBuffer
{ public:
   SampleBuffer()
     { Sample=0; AllocSize=Size=Read=Write=0; }
   ~SampleBuffer()
     { free(Sample); }
   void Free(void)
     { free(Sample); Sample=0; AllocSize=Size=Read=Write=0; }
   int EnsureSize(size_t MaxSize)
     { // printf(" SampleBuffer::EnsureSize(%d)\n",MaxSize);
       if(MaxSize<=AllocSize) return 0;
#ifdef __CINT__  // CINT doesn't accept NULL pointers to realloc()
       if(Sample) Sample=(Type *)realloc(Sample,MaxSize*sizeof(Type));
             else Sample=(Type *)malloc(MaxSize*sizeof(Type));
#else
       Sample=(Type *)realloc(Sample,MaxSize*sizeof(Type));
#endif
       if(Sample==0) { AllocSize=Size=0; return -1; }
       AllocSize=MaxSize; return 0; }
   int EnsureWriteLen(size_t MaxWriteLen)  // ensure enough space for writing new data
     { return EnsureSize(Write+MaxWriteLen); }
   void SetEmpty(void)                  // remove all data, make buffer empty
     { Read=Write=Size=0; }
   void RemovePastData(void)            // remove data before the read pointer
     { memcpy(Sample,Sample+Read,(Size-Read)*sizeof(Type));
       Write-=Read; Size-=Read; Read=0; }
   Type &operator[] (size_t Idx)        // access data given by index
     { return Sample[Idx]; }
   size_t ReadLen(void)                 // length of data to be read
     { return Write-Read; }
   Type *ReadPtr(void)                  // get pointer to read data
     { return Sample+Read; }
   size_t WriteLen(void)                // length of data that can be written   
     { return Size-Write; }
   Type *WritePtr(void)                 // get pointer to write data
     { return Sample+Write; }
   void Print(char *Title=0)
     { printf("%s Sample=%08X, Read=%d, Write=%d, Size=%d, AllocSize=%d\n",
              Title ? Title:"SampleBuffer:", (int)Sample, Read, Write, Size, AllocSize); }
  public:
   Type *Sample;      // allocated storage pointer
   size_t Read;       // index to data that is ready to be read
   size_t Write;      // index to data that is still to be written
   size_t Size;       // current size of data (includes data before the read pointer)
   size_t AllocSize;  // allocated size for data
} ;

// ========================================================================

#endif // __STRUC_H__

