#include "PETSc.hpp"

#ifdef WITH_slepc
#define WITH_SLEPC
#endif
#ifdef WITH_slepccomplex
#define WITH_SLEPC
#endif

#ifdef WITH_SLEPC

#include "slepc.h"

namespace SLEPc {
template<class K, typename std::enable_if<std::is_same<K, double>::value || !std::is_same<PetscScalar, double>::value>::type* = nullptr>
void copy(K* pt, PetscInt n, PetscScalar* xr, PetscScalar* xi) { }
template<class K, typename std::enable_if<!std::is_same<K, double>::value && std::is_same<PetscScalar, double>::value>::type* = nullptr>
void copy(K* pt, PetscInt n, PetscScalar* xr, PetscScalar* xi) {
    for(int i = 0; i < n; ++i)
        pt[i] = K(xr[i], xi[i]);
}
template<class K, typename std::enable_if<std::is_same<K, double>::value || !std::is_same<PetscScalar, double>::value>::type* = nullptr>
void assign(K* pt, PetscScalar& kr, PetscScalar& ki) {
    *pt = kr;
}
template<class K, typename std::enable_if<!std::is_same<K, double>::value && std::is_same<PetscScalar, double>::value>::type* = nullptr>
void assign(K* pt, PetscScalar& kr, PetscScalar& ki) {
    *pt = K(kr, ki);
}
template<class K, typename std::enable_if<(std::is_same<PetscScalar, double>::value && std::is_same<K, std::complex<double>>::value)>::type* = nullptr>
void distributedVec(unsigned int* num, unsigned int first, unsigned int last, K* const in, PetscScalar* pt, unsigned int n) { }
template<class K, typename std::enable_if<!(std::is_same<PetscScalar, double>::value && std::is_same<K, std::complex<double>>::value)>::type* = nullptr>
void distributedVec(unsigned int* num, unsigned int first, unsigned int last, K* const in, PetscScalar* pt, unsigned int n) {
    HPDDM::Subdomain<K>::template distributedVec<0>(num, first, last, in, pt, n, 1);
}
template<class Type, class K>
class eigensolver_Op : public E_F0mps {
    public:
        Expression A;
        Expression B;
        static const int n_name_param = 9;
        static basicAC_F0::name_and_type name_param[];
        Expression nargs[n_name_param];
        eigensolver_Op(const basicAC_F0& args, Expression param1, Expression param2) : A(param1), B(param2) {
            args.SetNameParam(n_name_param, name_param, nargs);
        }

        AnyType operator()(Stack stack) const;
};
template<class Type, class K>
basicAC_F0::name_and_type eigensolver_Op<Type, K>::name_param[] = {
    {"sparams", &typeid(std::string*)},
    {"prefix", &typeid(std::string*)},
    {"values", &typeid(KN<K>*)},
    {"vectors", &typeid(FEbaseArrayKn<K>*)},
    {"array", &typeid(KNM<K>*)},
    {"fields", &typeid(KN<double>*)},
    {"names", &typeid(KN<String>*)},
    {"schurPreconditioner", &typeid(KN<Matrice_Creuse<PetscScalar>>*)},
    {"schurList", &typeid(KN<double>*)}
};
template<class Type, class K>
class eigensolver : public OneOperator {
    public:
        K dummy;
        eigensolver() : OneOperator(atype<long>(), atype<Type*>(), atype<Type*>()) { }

        E_F0* code(const basicAC_F0& args) const {
            return new eigensolver_Op<Type, K>(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]));
        }
};
template<class Type, class K>
AnyType eigensolver_Op<Type, K>::operator()(Stack stack) const {
    Type* ptA = GetAny<Type*>((*A)(stack));
    Type* ptB = GetAny<Type*>((*B)(stack));
    if(ptA->_petsc) {
        EPS eps;
        EPSCreate(PETSC_COMM_WORLD, &eps);
        EPSSetOperators(eps, ptA->_petsc, ptB->_A ? ptB->_petsc : NULL);
        std::string* options = nargs[0] ? GetAny<std::string*>((*nargs[0])(stack)) : NULL;
        bool fieldsplit = PETSc::insertOptions(options);
        if(nargs[1])
            EPSSetOptionsPrefix(eps, GetAny<std::string*>((*nargs[1])(stack))->c_str());
        EPSSetFromOptions(eps);
        if(fieldsplit) {
            KN<double>* fields = nargs[5] ? GetAny<KN<double>*>((*nargs[5])(stack)) : 0;
            KN<String>* names = nargs[6] ? GetAny<KN<String>*>((*nargs[6])(stack)) : 0;
            KN<Matrice_Creuse<PetscScalar>>* mS = nargs[7] ? GetAny<KN<Matrice_Creuse<PetscScalar>>*>((*nargs[7])(stack)) : 0;
            KN<double>* pL = nargs[8] ? GetAny<KN<double>*>((*nargs[8])(stack)) : 0;
            if(fields && names) {
                ST st;
                KSP ksp;
                PC pc;
                EPSGetST(eps, &st);
                STGetKSP(st, &ksp);
                KSPSetOperators(ksp, ptA->_petsc, ptA->_petsc);
                setFieldSplitPC(ptA, ksp, fields, names, mS, pL);
                EPSSetUp(eps);
                if(!ptA->_S.empty()) {
                    KSPGetPC(ksp, &pc);
                    PCSetUp(pc);
                    setCompositePC(ptA, pc);
                }
            }
        }
        FEbaseArrayKn<K>* eigenvectors = nargs[3] ? GetAny<FEbaseArrayKn<K>*>((*nargs[3])(stack)) : nullptr;
        Vec* basis = nullptr;
        PetscInt n = 0;
        if(eigenvectors && eigenvectors->N > 0 && eigenvectors->get(0) && eigenvectors->get(0)->n > 0) {
            n = eigenvectors->N;
            basis = new Vec[n];
            for(int i = 0; i < n; ++i) {
                MatCreateVecs(ptA->_petsc, &basis[i], NULL);
                PetscScalar* pt;
                VecGetArray(basis[i], &pt);
                if(!(std::is_same<PetscScalar, double>::value && std::is_same<K, std::complex<double>>::value))
                    distributedVec(ptA->_num, ptA->_first, ptA->_last, static_cast<K*>(*(eigenvectors->get(i))), pt, eigenvectors->get(i)->n);
                VecRestoreArray(basis[i], &pt);
            }
        }
        eigenvectors->resize(0);
        if(n)
            EPSSetInitialSpace(eps, n, basis);
        EPSSolve(eps);
        for(int i = 0; i < n; ++i)
            VecDestroy(&basis[i]);
        delete [] basis;
        PetscInt nconv;
        EPSGetConverged(eps, &nconv);
        if(nconv > 0 && (nargs[2] || nargs[3])) {
            KN<K>* eigenvalues = nargs[2] ? GetAny<KN<K>*>((*nargs[2])(stack)) : nullptr;
            KNM<K>* array = nargs[4] ? GetAny<KNM<K>*>((*nargs[4])(stack)) : nullptr;
            if(eigenvalues)
                eigenvalues->resize(nconv);
            if(eigenvectors)
                eigenvectors->resize(nconv);
            if(array)
                array->resize(ptA->_A->getDof(), nconv);
            Vec xr, xi;
            PetscInt n;
            if(eigenvectors || array) {
                MatCreateVecs(ptA->_petsc, PETSC_NULL, &xr);
                MatCreateVecs(ptA->_petsc, PETSC_NULL, &xi);
                VecGetLocalSize(xr, &n);
            }
            for(PetscInt i = 0; i < nconv; ++i) {
                PetscScalar kr, ki;
                EPSGetEigenpair(eps, i, &kr, &ki, (eigenvectors || array) ? xr : NULL, (eigenvectors || array) && std::is_same<PetscScalar, double>::value && std::is_same<K, std::complex<double>>::value ? xi : NULL);
                if(eigenvectors || array) {
                    PetscScalar* tmpr;
                    PetscScalar* tmpi;
                    VecGetArray(xr, &tmpr);
                    K* pt;
                    if(std::is_same<PetscScalar, double>::value && std::is_same<K, std::complex<double>>::value) {
                        VecGetArray(xi, &tmpi);
                        pt = new K[n];
                        copy(pt, n, tmpr, tmpi);
                    }
                    else
                        pt = reinterpret_cast<K*>(tmpr);
                    KN<K> cpy(ptA->_A->getDof());
                    cpy = K(0.0);
                    HPDDM::Subdomain<K>::template distributedVec<1>(ptA->_num, ptA->_first, ptA->_last, static_cast<K*>(cpy), pt, cpy.n, 1);
                    ptA->_A->HPDDM::template Subdomain<PetscScalar>::exchange(static_cast<K*>(cpy));
                    if(eigenvectors)
                        eigenvectors->set(i, cpy);
                    if(array)
                        (*array)(':', i) = cpy;
                    if(std::is_same<PetscScalar, double>::value && std::is_same<K, std::complex<double>>::value)
                        delete [] pt;
                    else
                        VecRestoreArray(xi, &tmpi);
                    VecRestoreArray(xr, &tmpr);
                }
                if(eigenvalues) {
                    if(sizeof(PetscScalar) == sizeof(K))
                        (*eigenvalues)[i] = kr;
                    else
                        assign(static_cast<K*>(*eigenvalues + i), kr, ki);

                }
            }
            if(eigenvectors || array) {
                VecDestroy(&xr);
                VecDestroy(&xi);
            }
        }
        EPSDestroy(&eps);
        return static_cast<long>(nconv);
    }
    else
        return 0L;
}
void finalizeSLEPc() {
    PETSC_COMM_WORLD = MPI_COMM_WORLD;
    SlepcFinalize();
}
template<class K, typename std::enable_if<std::is_same<K, double>::value>::type* = nullptr>
void addSLEPc() {
    Global.Add("deigensolver", "(", new SLEPc::eigensolver<PETSc::DistributedCSR<HpSchwarz<PetscScalar>>, double>());
}
template<class K, typename std::enable_if<!std::is_same<K, double>::value>::type* = nullptr>
void addSLEPc() { }
}

static void Init() {
    //  to load only once
    aType t;
    int r;
#ifdef WITH_slepccomplex
    const char * mmmm= "Petsc Slepc complex";
#else
    const char * mmmm= "Petsc Slepc real";
#endif
    if(!zzzfff->InMotClef(mmmm,t,r))
    {
#ifdef PETScandSLEPc
        Init_PETSc();
#endif
        int argc = pkarg->n;
        char** argv = new char*[argc];
        for(int i = 0; i < argc; ++i)
            argv[i] = const_cast<char*>((*(*pkarg)[i].getap())->c_str());
        PetscBool isInitialized;
        PetscInitialized(&isInitialized);
        if(!isInitialized && mpirank == 0)
            std::cout << "PetscInitialize has not been called, do not forget to load PETSc before loading SLEPc" << std::endl;
        SlepcInitialize(&argc, &argv, 0, "");
        delete [] argv;
        ff_atend(SLEPc::finalizeSLEPc);
        SLEPc::addSLEPc<PetscScalar>();
        Global.Add("zeigensolver", "(", new SLEPc::eigensolver<PETSc::DistributedCSR<HpSchwarz<PetscScalar>>, std::complex<double>>());
        if(verbosity>1)cout << "*** End:: load PETSc & SELPc "<< typeid(PetscScalar).name() <<"\n\n"<<endl;
        zzzfff->Add(mmmm, atype<Dmat*>());
    }
    else {
        if(verbosity>1)cout << "*** reload and skip load PETSc & SELPc "<< typeid(PetscScalar).name() <<"\n\n"<<endl;
    }
}
#else
static void Init() {
     Init_PETSc();
}
#endif
