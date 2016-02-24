/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __handAnim_h__
#define __handAnim_h__

#include "basecpp.h" // some useful macros and functions

void   hand_anim_beg(const double* xs=NULL, const double* ys=NULL, int n=0);
void   hand_anim_end();

void   hand_anim_set(const double*  xs, const double*  ys, int n);
int    hand_anim_get(const double** xs, const double** ys);
int    hand_anim_get(double*        xs, double*        ys);

double hand_anim_get(double x); // returns y for given x

void   hand_anim_add       (double x, double y);
void   hand_anim_chg(int p, double x, double y);
void   hand_anim_del(int p);

#endif // __handAnim_h__

