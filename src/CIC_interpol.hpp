#pragma once


template<typename T, unsigned int order>
struct CIC
{
  T state_integrator[order];
  unsigned decim_counter,decim;

  T 
  
  CIC(unsigned decim):decim_counter(0),decim(decim)
  {
      for (int i = 0; i < order; i++)
      {
	state_integrator[i] = 0;
      }
  }

  void process(T *in_begin, T * in_end, T *out)
  {
    for (; in_begin < in_end; in_begin++,out++)
      {
	state_integrator[0] += *in_begin;
	for (unsigned int i = 1; i < order; i++)
	  {
	    state_integrator[i] += state_integrator[i-1];
	  }
	if (decim_counter == 0)
	  {
	    //out
	  }
	decim_counter = (decim_counter + 1) % decim;
      }
  }
};
