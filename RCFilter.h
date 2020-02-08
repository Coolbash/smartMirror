#ifdef SHOW_INCLUDES		//if defined, each *.h file message name in output Window
#pragma message ("  {//including "__FILE__)
#endif
#ifndef _RCFilter_
#define _RCFilter_
/*
 *$Header: /Medic@XAI/Version Main/REMET/Src/RCFilter.h 33    20.02.13 18:13 Kirichenko $
*/
#define _USE_MATH_DEFINES 
//#include <math.h>

//#include "RemetFilter.h"
//#include <rmtMath.h>
//#include "rmtRound.h"

//-------------------------------------------------------------------------
typedef float CSample;
//-------------------------------------------------------------------------
//класс, реализующий примитивную  RC фильтрацию
class CFilterRC //: public CFilter
{
public:
	//DECLARE_DYNAMIC(CFilterRC)
//protected:
	//float		m_tau; //Постоянная времени фильтра
	//float		m_dt;  //частота квантования
	double		m_Out=0; //Собственно напряжение на выходе
	float		m_K=0;
	//double		m_Tau; // only for information. Not for any calculation	

public:
	//CFilterRC(){m_Out=0; m_K=0; /*m_Tau=0;*/};
	
	void			SetParam(float Tau, float SamplePeriod) { /*m_Tau=Tau;*/ m_K=Tau !=0 ? SamplePeriod / Tau : m_Out=0;};
	void			SetParam_Freq(float rFreq, float SamplePeriod) { SetParam(M_PI_2/*rmt::Math::_PI_2f*//rFreq, SamplePeriod); };

	//virtual	void	GetFilterInfo(CFilter::CInfo &Info) const					{Info.m_nStreamFilterShift = Info.m_nBlockFilterShift = 0; Info.m_bIsModificationFilter	= true;};
};
//-------------------------------------------------------------------------

class CFilterRC_HiPass : public CFilterRC
{
public:
	//DECLARE_DYNAMIC(CFilterRC_HiPass)

public:
	CFilterRC_HiPass() {};
	CFilterRC_HiPass(float Tau, float SamplePeriod) { m_K=Tau != 0 ? SamplePeriod / Tau : 0; m_Out=0;	};
	
	virtual void	FilterSample(CSample &sample){sample = CSample(HiPass(sample));}
	//virtual	void	FilterSample(CSample * pSample, size_t nSize) {CFilterRC::FilterSample(pSample,nSize);}
	//virtual	void	FilterSample(const CSample * pSrcSample, CSample * pDstSample, size_t nSize) { CFilterRC::FilterSample(pSrcSample, pDstSample, nSize); }

public:
	double HiPass(double Src){return Src-(m_Out+=(Src-m_Out)*m_K);};
};
//-------------------------------------------------------------------------

class CFilterRC_LoPass : public CFilterRC
{
public:
	//DECLARE_DYNAMIC(CFilterRC_LoPass)

public:
	virtual void	FilterSample(CSample &sample){sample = CSample(LoPass(sample));}	//{sample=(m_Out+=(sample-m_Out)*m_K);};//round_safe(m_Out+=(sample-m_Out)*m_K,sample);
	//virtual	void	FilterSample(CSample * pSample, size_t nSize) { CFilterRC::FilterSample(pSample, nSize); }
	//virtual	void	FilterSample(const CSample * pSrcSample, CSample * pDstSample, size_t nSize) { CFilterRC::FilterSample(pSrcSample, pDstSample, nSize); }

public:
	double LoPass(double Src){return m_Out+=(Src-m_Out)*m_K;};

	//virtual	void	GetFilterInfo(CFilter::CInfo &Info) const					{UINT Shift= Round(m_Tau* M_PI_4 /*rmt::Math::_PI_4f*/); Info.m_nStreamFilterShift = Info.m_nBlockFilterShift = Shift; Info.m_bIsModificationFilter	= true;};
};
//-------------------------------------------------------------------------


#else
#ifdef SHOW_ALREADY_INCLUDED
#pragma message("... "__FILE__" already included!")
#endif
#endif  //_RCFilter_

#ifdef SHOW_INCLUDES		//if defined, each *.h file message name in output Window
#pragma message ("  }//included "__FILE__)
#endif