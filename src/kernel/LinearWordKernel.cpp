#include "lib/common.h"
#include "lib/Mathmatics.h"
#include "kernel/LinearWordKernel.h"
#include "kernel/WordKernel.h"
#include "features/WordFeatures.h"
#include "lib/io.h"

#include <assert.h>

CLinearWordKernel::CLinearWordKernel(LONG size)
  : CWordKernel(size),scale(1.0),normal(NULL)
{
}

CLinearWordKernel::~CLinearWordKernel() 
{
	cleanup();
}

bool CLinearWordKernel::init(CFeatures* l, CFeatures* r, bool do_init)
{
	CWordKernel::init(l, r, do_init); 

	if (do_init)
		init_rescale() ;

	CIO::message(M_INFO, "rescaling kernel by %g (num:%d)\n",scale, CMath::min(l->get_num_vectors(), r->get_num_vectors()));

	return true;
}

void CLinearWordKernel::init_rescale()
{
	LONGREAL sum=0;
	scale=1.0;
	for (INT i=0; (i<lhs->get_num_vectors() && i<rhs->get_num_vectors()); i++)
		sum+=compute(i, i);

	if ( sum > (pow((double) 2, (double) 8*sizeof(LONG))) )
		CIO::message(M_ERROR, "the sum %lf does not fit into integer of %d bits expect bogus results.\n", sum, 8*sizeof(LONG));
	scale=sum/CMath::min(lhs->get_num_vectors(), rhs->get_num_vectors());
}

void CLinearWordKernel::cleanup()
{
	delete_optimization();
}

bool CLinearWordKernel::load_init(FILE* src)
{
	return false;
}

bool CLinearWordKernel::save_init(FILE* dest)
{
	return false;
}

void CLinearWordKernel::clear_normal()
{
	int num = lhs->get_num_vectors();

	for (int i=0; i<num; i++)
		normal[i]=0;
}

void CLinearWordKernel::add_to_normal(INT idx, REAL weight) 
{
	INT vlen;
	bool vfree;
	WORD* vec=((CWordFeatures*) lhs)->get_feature_vector(idx, vlen, vfree);

	for (int i=0; i<vlen; i++)
		normal[i]+= weight*vec[i];

	((CWordFeatures*) lhs)->free_feature_vector(vec, idx, vfree);
}
  
REAL CLinearWordKernel::compute(INT idx_a, INT idx_b)
{
	INT alen, blen;
	bool afree, bfree;

	WORD* avec=((CWordFeatures*) lhs)->get_feature_vector(idx_a, alen, afree);
	WORD* bvec=((CWordFeatures*) rhs)->get_feature_vector(idx_b, blen, bfree);

	assert(alen==blen);
	double sum=0;
	for (LONG i=0; i<alen; i++)
		sum+=((LONG) avec[i])*((LONG) bvec[i]);

	REAL result=sum/scale;
	((CWordFeatures*) lhs)->free_feature_vector(avec, idx_a, afree);
	((CWordFeatures*) rhs)->free_feature_vector(bvec, idx_b, bfree);

	return result;
}

bool CLinearWordKernel::init_optimization(INT num_suppvec, INT* sv_idx, REAL* alphas) 
{
	CIO::message(M_DEBUG,"drin gelandet yeah\n");
	INT alen;
	bool afree;
	int i;

	int num_feat=((CWordFeatures*) lhs)->get_num_features();
	assert(num_feat);

	normal=new REAL[num_feat];
	assert(normal);

	for (i=0; i<num_feat; i++)
		normal[i]=0;

	for (int i=0; i<num_suppvec; i++)
	{
		WORD* avec=((CWordFeatures*) lhs)->get_feature_vector(sv_idx[i], alen, afree);
		assert(avec);

		for (int j=0; j<num_feat; j++)
			normal[j]+=alphas[i] * ((double) avec[j]);

		((CWordFeatures*) lhs)->free_feature_vector(avec, 0, afree);
	}

	set_is_initialized(true);
	return true;
}

bool CLinearWordKernel::delete_optimization()
{
	delete[] normal;
	normal=NULL;
	set_is_initialized(false);

	return true;
}

REAL CLinearWordKernel::compute_optimized(INT idx_b) 
{
	INT blen;
	bool bfree;

	WORD* bvec=((CWordFeatures*) rhs)->get_feature_vector(idx_b, blen, bfree);

	double result=0;
	{
		for (INT i=0; i<blen; i++)
			result+= normal[i] * ((double) bvec[i]);
	}
	result/=scale;

	((CWordFeatures*) rhs)->free_feature_vector(bvec, idx_b, bfree);

	return result;
}
