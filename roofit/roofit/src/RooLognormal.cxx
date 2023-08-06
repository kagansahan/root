 /*****************************************************************************
  * Project: RooFit                                                           *
  * @(#)root/roofit:$Id$ *
  *                                                                           *
  * RooFit Lognormal PDF                                                      *
  *                                                                           *
  * Author: Gregory Schott and Stefan Schmitz                                 *
  *                                                                           *
  *****************************************************************************/

/** \class RooLognormal
    \ingroup Roofit

RooFit Lognormal PDF. The two parameters are:
  - `m0`: the median of the distribution
  - `k = exp(sigma)`: sigma is called the shape parameter in the TMath parameterization

\f[
  \mathrm{RooLognormal}(x \, | \, m_0, k) = \frac{1}{\sqrt{2\pi \cdot \ln(k) \cdot x}} \cdot \exp\left( \frac{-\ln^2(\frac{x}{m_0})}{2 \ln^2(k)} \right)
\f]

The parameterization here is physics driven and differs from the ROOT::Math::lognormal_pdf() in `x,m,s,x0` with:
  - `m = log(m0)`
  - `s = log(k)`
  - `x0 = 0`
**/

#include "RooLognormal.h"
#include "RooRandom.h"
#include "RooMath.h"
#include "RooHelpers.h"
#include "RooBatchCompute.h"

#include <Math/PdfFuncMathCore.h>

ClassImp(RooLognormal);

////////////////////////////////////////////////////////////////////////////////

RooLognormal::RooLognormal(const char *name, const char *title,
          RooAbsReal& _x, RooAbsReal& _m0,
          RooAbsReal& _k) :
  RooAbsPdf(name,title),
  x("x","Observable",this,_x),
  m0("m0","m0",this,_m0),
  k("k","k",this,_k)
{
    RooHelpers::checkRangeOfParameters(this, {&_x, &_m0, &_k}, 0.);

    auto par = dynamic_cast<const RooAbsRealLValue*>(&_k);
    if (par && par->getMin()<=1 && par->getMax()>=1 ) {
      coutE(InputArguments) << "The parameter '" << par->GetName() << "' with range [" << par->getMin("") << ", "
          << par->getMax() << "] of the " << this->ClassName() << " '" << this->GetName()
          << "' can reach the unsafe value 1.0 " << ". Advise to limit its range." << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////

RooLognormal::RooLognormal(const RooLognormal& other, const char* name) :
  RooAbsPdf(other,name), x("x",this,other.x), m0("m0",this,other.m0),
  k("k",this,other.k)
{
}

////////////////////////////////////////////////////////////////////////////////
/// ln(k)<1 would correspond to sigma < 0 in the parameterization
/// resulting by transforming a normal random variable in its
/// standard parameterization to a lognormal random variable
/// => treat ln(k) as -ln(k) for k<1

double RooLognormal::evaluate() const
{
  double ln_k = TMath::Abs(TMath::Log(k));
  double ln_m0 = TMath::Log(m0);

  double ret = ROOT::Math::lognormal_pdf(x,ln_m0,ln_k);
  return ret ;
}

void RooLognormal::translate(RooFit::Detail::CodeSquashContext &ctx) const
{
   ctx.addResult(this, ctx.buildCall("RooFit::Detail::EvaluateFuncs::logNormalEvaluate", x, k, m0));
}

////////////////////////////////////////////////////////////////////////////////
/// Compute multiple values of Lognormal distribution.
void RooLognormal::computeBatch(double* output, size_t nEvents, RooFit::Detail::DataMap const& dataMap) const
{
   RooBatchCompute::compute(dataMap.config(this), RooBatchCompute::Lognormal, output, nEvents,
          {dataMap.at(x), dataMap.at(m0), dataMap.at(k)});
}

////////////////////////////////////////////////////////////////////////////////

Int_t RooLognormal::getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars, const char* /*rangeName*/) const
{
  if (matchArgs(allVars,analVars,x)) return 1 ;
  return 0 ;
}

////////////////////////////////////////////////////////////////////////////////

double RooLognormal::analyticalIntegral(Int_t code, const char* rangeName) const
{
  R__ASSERT(code==1) ;

  static const double root2 = sqrt(2.) ;

  double ln_k = TMath::Abs(TMath::Log(k));
  double ret = 0.5*( RooMath::erf( TMath::Log(x.max(rangeName)/m0)/(root2*ln_k) ) - RooMath::erf( TMath::Log(x.min(rangeName)/m0)/(root2*ln_k) ) ) ;

  return ret ;
}

std::string
RooLognormal::buildCallToAnalyticIntegral(int code, const char *rangeName, RooFit::Detail::CodeSquashContext &ctx) const
{
   R__ASSERT(code == 1);

   return ctx.buildCall("RooFit::Detail::AnalyticalIntegrals::logNormalIntegral", x.min(rangeName), x.max(rangeName),
                        m0, k);
}

////////////////////////////////////////////////////////////////////////////////

Int_t RooLognormal::getGenerator(const RooArgSet& directVars, RooArgSet &generateVars, bool /*staticInitOK*/) const
{
  if (matchArgs(directVars,generateVars,x)) return 1 ;
  return 0 ;
}

////////////////////////////////////////////////////////////////////////////////

void RooLognormal::generateEvent(Int_t code)
{
  R__ASSERT(code==1) ;

  double xgen ;
  while(true) {
    xgen = TMath::Exp(RooRandom::randomGenerator()->Gaus(TMath::Log(m0),TMath::Log(k)));
    if (xgen<=x.max() && xgen>=x.min()) {
      x = xgen ;
      break;
    }
  }

  return;
}
