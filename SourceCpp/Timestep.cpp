#include "Timestep.H"

// EstDt routines
namespace TimeStep {
AMREX_GPU_DEVICE_MANAGED amrex::Real max_dt =
  std::numeric_limits<amrex::Real>::max();
} // namespace TimeStep

AMREX_GPU_DEVICE
amrex::Real
pc_estdt_hydro(
  const amrex::Box& bx,
  const amrex::Array4<const amrex::Real>& u,
#ifdef PELEC_USE_EB
  const amrex::Array4<const amrex::EBCellFlag>& flags,
#endif
  AMREX_D_DECL(
    const amrex::Real& dx,
    const amrex::Real& dy,
    const amrex::Real& dz)) noexcept
{
  amrex::Real dt = TimeStep::max_dt;

  amrex::Loop(bx, [=, &dt](int i, int j, int k) {
#ifdef PELEC_USE_EB
    if (not flags(i, j, k).isCovered()) {
#endif
      const amrex::Real rho = u(i, j, k, URHO);
      const amrex::Real rhoInv = 1.0 / rho;
      amrex::Real T = u(i, j, k, UTEMP);
      amrex::Real massfrac[NUM_SPECIES];
      amrex::Real c;
      for (int n = 0; n < NUM_SPECIES; ++n) {
        massfrac[n] = u(i, j, k, UFS + n) * rhoInv;
      }
      EOS::RTY2Cs(rho, T, massfrac, c);
      AMREX_D_TERM(const amrex::Real ux = u(i, j, k, UMX) * rhoInv;
                   const amrex::Real dt1 = dx / (c + amrex::Math::abs(ux));
                   dt = amrex::min<amrex::Real>(dt, dt1);
                   , const amrex::Real uy = u(i, j, k, UMY) * rhoInv;
                   const amrex::Real dt2 = dy / (c + amrex::Math::abs(uy));
                   dt = amrex::min<amrex::Real>(dt, dt2);
                   , const amrex::Real uz = u(i, j, k, UMZ) * rhoInv;
                   const amrex::Real dt3 = dz / (c + amrex::Math::abs(uz));
                   dt = amrex::min<amrex::Real>(dt, dt3););
#ifdef PELEC_USE_EB
    }
#endif
  });
  return dt;
}

// Diffusion Velocity
AMREX_GPU_DEVICE
amrex::Real
pc_estdt_veldif(
  const amrex::Box& bx,
  const amrex::Array4<const amrex::Real>& u,
#ifdef PELEC_USE_EB
  const amrex::Array4<const amrex::EBCellFlag>& flags,
#endif
  AMREX_D_DECL(
    const amrex::Real& dx, const amrex::Real& dy, const amrex::Real& dz),
  TransParm const* trans_parm) noexcept
{
  amrex::Real dt = TimeStep::max_dt;

  amrex::Loop(bx, [=, &dt](int i, int j, int k) {
#ifdef PELEC_USE_EB
    if (not flags(i, j, k).isCovered()) {
#endif
      const amrex::Real rho = u(i, j, k, URHO);
      const amrex::Real rhoInv = 1.0 / rho;
      amrex::Real massfrac[NUM_SPECIES];
      for (int n = 0; n < NUM_SPECIES; ++n) {
        massfrac[n] = u(i, j, k, n + UFS) * rhoInv;
      }
      amrex::Real T = u(i, j, k, UTEMP);
      amrex::Real D = 0.0;
      const int which_trans = 0;
      pc_trans4dt(which_trans, T, rho, massfrac, D, trans_parm);
      D *= rhoInv;
      if (D == 0.0) {
        D = SMALL_NUM;
      }
      AMREX_D_TERM(
        const amrex::Real dt1 = 0.5 * dx * dx / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt1);
        , const amrex::Real dt2 = 0.5 * dy * dy / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt2);
        , const amrex::Real dt3 = 0.5 * dz * dz / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt3););
#ifdef PELEC_USE_EB
    }
#endif
  });
  return dt;
}

// Diffusion Temperature
AMREX_GPU_DEVICE
amrex::Real
pc_estdt_tempdif(
  const amrex::Box& bx,
  const amrex::Array4<const amrex::Real>& u,
#ifdef PELEC_USE_EB
  const amrex::Array4<const amrex::EBCellFlag>& flags,
#endif
  AMREX_D_DECL(
    const amrex::Real& dx, const amrex::Real& dy, const amrex::Real& dz),
  TransParm const* trans_parm) noexcept
{
  amrex::Real dt = TimeStep::max_dt;

  amrex::Loop(bx, [=, &dt](int i, int j, int k) {
#ifdef PELEC_USE_EB
    if (not flags(i, j, k).isCovered()) {
#endif
      const amrex::Real rho = u(i, j, k, URHO);
      const amrex::Real rhoInv = 1.0 / rho;
      amrex::Real massfrac[NUM_SPECIES];
      for (int n = 0; n < NUM_SPECIES; ++n) {
        massfrac[n] = u(i, j, k, n + UFS) * rhoInv;
      }
      amrex::Real T = u(i, j, k, UTEMP);
      amrex::Real D = 0.0;
      const int which_trans = 1;
      pc_trans4dt(which_trans, T, rho, massfrac, D, trans_parm);
      amrex::Real cv;
      EOS::TY2Cv(T, massfrac, cv);
      D *= rhoInv / cv;
      if (D == 0.0) {
        D = SMALL_NUM;
      }
      AMREX_D_TERM(
        const amrex::Real dt1 = 0.5 * dx * dx / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt1);
        , const amrex::Real dt2 = 0.5 * dy * dy / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt2);
        , const amrex::Real dt3 = 0.5 * dz * dz / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt3););
#ifdef PELEC_USE_EB
    }
#endif
  });
  return dt;
}

// Diffusion Enthalpy
AMREX_GPU_DEVICE
amrex::Real
pc_estdt_enthdif(
  const amrex::Box& bx,
  const amrex::Array4<const amrex::Real>& u,
#ifdef PELEC_USE_EB
  const amrex::Array4<const amrex::EBCellFlag>& flags,
#endif
  AMREX_D_DECL(
    const amrex::Real& dx, const amrex::Real& dy, const amrex::Real& dz),
  TransParm const* trans_parm) noexcept
{
  amrex::Real dt = TimeStep::max_dt;

  amrex::Loop(bx, [=, &dt](int i, int j, int k) {
#ifdef PELEC_USE_EB
    if (not flags(i, j, k).isCovered()) {
#endif
      const amrex::Real rho = u(i, j, k, URHO);
      const amrex::Real rhoInv = 1.0 / rho;
      amrex::Real massfrac[NUM_SPECIES];
      for (int n = 0; n < NUM_SPECIES; ++n) {
        massfrac[n] = u(i, j, k, n + UFS) * rhoInv;
      }
      amrex::Real T = u(i, j, k, UTEMP);
      amrex::Real cp;
      EOS::TY2Cp(T, massfrac, cp);
      amrex::Real D;
      const int which_trans = 1;
      pc_trans4dt(which_trans, T, rho, massfrac, D, trans_parm);
      D *= rhoInv / cp;
      AMREX_D_TERM(
        const amrex::Real dt1 = 0.5 * dx * dx / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt1);
        , const amrex::Real dt2 = 0.5 * dy * dy / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt2);
        , const amrex::Real dt3 = 0.5 * dz * dz / (AMREX_SPACEDIM * D);
        dt = amrex::min<amrex::Real>(dt, dt3););

#ifdef PELEC_USE_EB
    }
#endif
  });
  return dt;
}
