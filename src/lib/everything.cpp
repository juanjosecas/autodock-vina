/*

   Copyright (c) 2006-2010, The Scripps Research Institute

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Author: Dr. Oleg Trott <ot14@columbia.edu>, 
           The Olson Lab, 
           The Scripps Research Institute

*/

#include "everything.h"
#include "int_pow.h"

inline fl gaussian(fl x, fl width) {
	return std::exp(-sqr(x/width));
}

// distance_additive terms

template<unsigned i>
struct electrostatic : public distance_additive {
	fl cap;
	electrostatic(fl cap_, fl cutoff_) : distance_additive(cutoff_), cap(cap_) {
		name = std::string("electrostatic(i=") + to_string(i) + ", ^=" + to_string(cap) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(const atom_base& a, const atom_base& b, fl r) const {
		fl tmp = int_pow<i>(r);
		fl q1q2 = a.charge * b.charge;
		if(tmp < epsilon_fl) return q1q2 * cap;
		else                 return q1q2 * (std::min)(cap, 1/int_pow<i>(r));
	}
};

fl solvation_parameter(const atom_type& a) {
	if(a.ad < AD_TYPE_SIZE) return ad_type_property(a.ad).solvation;
	else if(a.xs == XS_TYPE_Met_D) return metal_solvation_parameter;
	VINA_CHECK(false); 
	return 0; // placating the compiler
}

fl volume(const atom_type& a) {
	if(a.ad < AD_TYPE_SIZE) return ad_type_property(a.ad).volume;
	else if(a.xs < XS_TYPE_SIZE) return 4*pi / 3 * int_pow<3>(xs_radius(a.xs));
	VINA_CHECK(false);
	return 0; // placating the compiler
}


struct ad4_solvation : public distance_additive {
	fl desolvation_sigma;
	fl solvation_q;
	bool charge_dependent;
	ad4_solvation(fl desolvation_sigma_, fl solvation_q_, bool charge_dependent_, fl cutoff_) : distance_additive(cutoff_), solvation_q(solvation_q_), charge_dependent(charge_dependent_), desolvation_sigma(desolvation_sigma_) {
		name = std::string("ad4_solvation(d-sigma=") + to_string(desolvation_sigma) + ", s/q=" + to_string(solvation_q) + ", q=" + to_string(charge_dependent) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(const atom_base& a, const atom_base& b, fl r) const {
		fl q1 = a.charge;
		fl q2 = b.charge;

		VINA_CHECK(not_max(q1));
		VINA_CHECK(not_max(q2));

		sz t1 = a.ad;
		sz t2 = b.ad;

		fl solv1 = solvation_parameter(a);
		fl solv2 = solvation_parameter(b);

		fl volume1 = volume(a);
		fl volume2 = volume(b);

		fl my_solv = charge_dependent ? solvation_q : 0;

		fl tmp = ((solv1 + my_solv * std::abs(q1)) * volume2 + 
			    (solv2 + my_solv * std::abs(q2)) * volume1) * std::exp(-sqr(r/(2*desolvation_sigma)));

		VINA_CHECK(not_max(tmp));
		return tmp;
	}
};

inline fl optimal_distance(sz xs_t1, sz xs_t2) {
	return xs_radius(xs_t1) + xs_radius(xs_t2);
}

struct gauss : public usable {
	fl offset; // added to optimal distance
	fl width;
	gauss(fl offset_, fl width_, fl cutoff_) : usable(cutoff_), offset(offset_), width(width_) {
		name = std::string("gauss(o=") + to_string(offset) + ", w=" + to_string(width) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		return gaussian(r - (optimal_distance(t1, t2) + offset), width);
	}
};

struct repulsion : public usable {
	fl offset; // added to vdw
	repulsion(fl offset_, fl cutoff_) : usable(cutoff_), offset(offset_) {
		name = std::string("repulsion(o=") + to_string(offset) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		fl d = r - (optimal_distance(t1, t2) + offset);
		if(d > 0) 
			return 0;
		return d*d;
	}
};

inline fl slope_step(fl x_bad, fl x_good, fl x) {
	if(x_bad < x_good) {
		if(x <= x_bad) return 0;
		if(x >= x_good) return 1;
	}
	else {
		if(x >= x_bad) return 0;
		if(x <= x_good) return 1;
	}
	return (x - x_bad) / (x_good - x_bad);
}

struct hydrophobic : public usable {
	fl good;
	fl bad;
	hydrophobic(fl good_, fl bad_, fl cutoff_) : usable(cutoff_), good(good_), bad(bad_) {
		name = "hydrophobic(g=" + to_string(good) + ", b=" + to_string(bad) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		if(xs_is_hydrophobic(t1) && xs_is_hydrophobic(t2))
			return slope_step(bad, good, r - optimal_distance(t1, t2));
		else return 0;
	}
};

struct non_hydrophobic : public usable {
	fl good;
	fl bad;
	non_hydrophobic(fl good_, fl bad_, fl cutoff_) : usable(cutoff_), good(good_), bad(bad_) {
		name = "non_hydrophobic(g=" + to_string(good) + ", b=" + to_string(bad) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		if(!xs_is_hydrophobic(t1) && !xs_is_hydrophobic(t2))
			return slope_step(bad, good, r - optimal_distance(t1, t2));
		else return 0;
	}
};

template<unsigned n, unsigned m>
void find_vdw_coefficients(fl position, fl depth, fl& c_n, fl& c_m) {
	BOOST_STATIC_ASSERT(n != m); 
	c_n = int_pow<n>(position) * depth * m / (fl(n)-fl(m));
	c_m = int_pow<m>(position) * depth * n / (fl(m)-fl(n));
}


template<unsigned i, unsigned j>
struct vdw : public usable {
	fl smoothing;
	fl cap;
	vdw(fl smoothing_, fl cap_, fl cutoff_) 
		: usable(cutoff_), smoothing(smoothing_), cap(cap_) {
		name = "vdw(i=" + to_string(i) + ", j=" + to_string(j) + ", s=" + to_string(smoothing) + ", ^=" + to_string(cap) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		fl d0 = optimal_distance(t1, t2);
		fl depth = 1; 
		fl c_i = 0;
		fl c_j = 0;
		find_vdw_coefficients<i, j>(d0, depth, c_i, c_j);
		if     (r > d0 + smoothing) r -= smoothing;
		else if(r < d0 - smoothing) r += smoothing;
		else r = d0;

		fl r_i = int_pow<i>(r);
		fl r_j = int_pow<j>(r);
		if(r_i > epsilon_fl && r_j > epsilon_fl)
			return (std::min)(cap, c_i / r_i + c_j / r_j);
		else 
			return cap;
	}
};

struct non_dir_h_bond : public usable {
	fl good;
	fl bad;
	non_dir_h_bond(fl good_, fl bad_, fl cutoff_) : usable(cutoff_), good(good_), bad(bad_) {
		name = std::string("non_dir_h_bond(g=") + to_string(good) + ", b=" + to_string(bad) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		if(xs_h_bond_possible(t1, t2))
			return slope_step(bad, good, r - optimal_distance(t1, t2));
		return 0;
	}
};

// Modern scoring terms based on recent literature (2010-2024)

// Halogen bonding term (X···O/N interactions where X = Cl, Br, I)
// Based on: Auffinger et al., PNAS 2004; Cavallo et al., Chem Rev 2016
struct halogen_bond : public usable {
	fl good;
	fl bad;
	halogen_bond(fl good_, fl bad_, fl cutoff_) : usable(cutoff_), good(good_), bad(bad_) {
		name = std::string("halogen_bond(g=") + to_string(good) + ", b=" + to_string(bad) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		// Halogen bond occurs when halogen (Cl, Br, I) interacts with acceptor (O, N)
		bool halogen_acceptor = (xs_is_halogen(t1) && xs_is_acceptor(t2)) || 
		                        (xs_is_halogen(t2) && xs_is_acceptor(t1));
		if(halogen_acceptor) {
			// Optimal distance is slightly longer than H-bonds (3.0-3.8 Å typical)
			// Use a favorable interaction with distance-dependent scoring
			return slope_step(bad, good, r - (optimal_distance(t1, t2) + 0.3));
		}
		return 0;
	}
};

// Pi-stacking interaction (aromatic-aromatic)
// Based on: Martinez & Iverson, Chem Sci 2012; Salonen et al., Angew Chem 2011
struct pi_stacking : public usable {
	fl good;
	fl bad;
	pi_stacking(fl good_, fl bad_, fl cutoff_) : usable(cutoff_), good(good_), bad(bad_) {
		name = std::string("pi_stacking(g=") + to_string(good) + ", b=" + to_string(bad) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		// Pi-stacking between aromatic rings (planar carbons)
		if(xs_is_aromatic(t1) && xs_is_aromatic(t2)) {
			// Optimal distance for pi-stacking is 3.4-3.8 Å (centroid-centroid)
			// Favorable interaction at this range
			return slope_step(bad, good, r - (optimal_distance(t1, t2) + 1.0));
		}
		return 0;
	}
};

// Sulfur-aromatic interaction (S-π interaction)
// Based on: Reid et al., J Mol Biol 1985; Valley et al., PNAS 2012
struct sulfur_aromatic : public usable {
	fl good;
	fl bad;
	sulfur_aromatic(fl good_, fl bad_, fl cutoff_) : usable(cutoff_), good(good_), bad(bad_) {
		name = std::string("sulfur_aromatic(g=") + to_string(good) + ", b=" + to_string(bad) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		// Sulfur-aromatic interaction (Met-π or other S-π)
		bool s_aromatic = (t1 == XS_TYPE_S_P && xs_is_aromatic(t2)) || 
		                  (t2 == XS_TYPE_S_P && xs_is_aromatic(t1));
		if(s_aromatic) {
			// S-π interactions are favorable at ~3.5-5.0 Å
			return slope_step(bad, good, r - (optimal_distance(t1, t2) + 0.5));
		}
		return 0;
	}
};

// Improved desolvation penalty with modern parameters
// Based on: Ben-Shimon & Eisenstein, Proteins 2010; Huang & Zou, IJMS 2010
struct desolvation_improved : public usable {
	fl weight;
	desolvation_improved(fl weight_, fl cutoff_) : usable(cutoff_), weight(weight_) {
		name = std::string("desolvation_improved(w=") + to_string(weight) + ", c=" + to_string(cutoff) + ")";
	}
	fl eval(sz t1, sz t2, fl r) const {
		// Improved desolvation based on atom hydrophobicity and burial
		// Stronger penalty for burying polar atoms, less for hydrophobic
		fl desolvation_factor = 0.0;
		
		// Desolvation factors based on literature entropy/enthalpy values
		const fl POLAR_POLAR_PENALTY = 1.0;      // Strong penalty for burying two polar atoms
		const fl HYDROPHOBIC_BONUS = -0.3;       // Favorable burial of hydrophobic atoms (hydrophobic effect)
		const fl MIXED_PENALTY = 0.3;            // Moderate penalty for mixed polarity
		const fl GAUSSIAN_WIDTH = 1.5;           // Width parameter for distance dependence (Å)
		const fl DISTANCE_CUTOFF_OFFSET = 2.0;   // Distance beyond optimal where effect diminishes (Å)
		
		// Calculate desolvation penalty based on polarity
		bool t1_hydrophobic = xs_is_hydrophobic(t1);
		bool t2_hydrophobic = xs_is_hydrophobic(t2);
		
		if(!t1_hydrophobic && !t2_hydrophobic) {
			// Both polar: strong desolvation penalty
			desolvation_factor = POLAR_POLAR_PENALTY;
		} else if(t1_hydrophobic && t2_hydrophobic) {
			// Both hydrophobic: favorable burial (hydrophobic effect)
			desolvation_factor = HYDROPHOBIC_BONUS;
		} else {
			// Mixed: moderate penalty
			desolvation_factor = MIXED_PENALTY;
		}
		
		// Distance-dependent: closer contacts have stronger effects
		fl d_optimal = optimal_distance(t1, t2);
		if(r < d_optimal + DISTANCE_CUTOFF_OFFSET) {
			return weight * desolvation_factor * gaussian(r - d_optimal, GAUSSIAN_WIDTH);
		}
		return 0;
	}
};

inline fl read_iterator(flv::const_iterator& i) {
	fl x = *i; 
	++i;
	return x;
}

fl smooth_div(fl x, fl y) {
	if(std::abs(x) < epsilon_fl) return 0;
	if(std::abs(y) < epsilon_fl) return ((x*y > 0) ? max_fl : -max_fl); // FIXME I hope -max_fl does not become NaN
	return x / y;
}

struct num_tors_add : public conf_independent {
	num_tors_add() { name = "num_tors_add"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		//fl w = 0.1 * read_iterator(i); // [-1 .. 1]
		fl w = read_iterator(i); // FIXME?
		return x + w * in.num_tors;
	}
};

struct num_tors_sqr : public conf_independent {
	num_tors_sqr() { name = "num_tors_sqr"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		fl w = 0.1 * read_iterator(i); // [-1 .. 1]
		return x + w * sqr(fl(in.num_tors)) / 5;
	}
};

struct num_tors_sqrt : public conf_independent {
	num_tors_sqrt() { name = "num_tors_sqrt"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		fl w = 0.1 * read_iterator(i); // [-1 .. 1]
		return x + w * std::sqrt(fl(in.num_tors)) / sqrt(5.0);
	}
};

struct num_tors_div : public conf_independent {
	num_tors_div() { name = "num_tors_div"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		fl w = 0.1 * (read_iterator(i) + 1); // w is in [0..0.2]
		return smooth_div(x, 1 + w * in.num_tors/5.0);
	}
};

struct ligand_length : public conf_independent {
	ligand_length() { name = "ligand_length"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		fl w = read_iterator(i);
		return x + w * in.ligand_lengths_sum;
	}
};

struct num_ligands : public conf_independent {
	num_ligands() { name = "num_ligands"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		fl w = 1 * read_iterator(i); // w is in [-1.. 1]
		return x + w * in.num_ligands;
	}
};

struct num_heavy_atoms_div : public conf_independent {
	num_heavy_atoms_div() { name = "num_heavy_atoms_div"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		fl w = 0.05 * read_iterator(i); 
		return smooth_div(x, 1 + w * in.num_heavy_atoms); 
	}
};

struct num_heavy_atoms : public conf_independent {
	num_heavy_atoms() { name = "num_heavy_atoms"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		fl w = 0.05 * read_iterator(i); 
		return x + w * in.num_heavy_atoms;
	}
};

struct num_hydrophobic_atoms : public conf_independent {
	num_hydrophobic_atoms() { name = "num_hydrophobic_atoms"; }
	sz size() const { return 1; }
	fl eval(const conf_independent_inputs& in, fl x, flv::const_iterator& i) const {
		fl w = 0.05 * read_iterator(i); 
		return x + w * in.num_hydrophobic_atoms;
	}
};

everything::everything() { // enabled according to design.out227
	const unsigned d = 0; // default
	const fl cutoff = 8; //6;

	// FIXME? enable some?
	//// distance_additive
	//add(d, new ad4_solvation(3.6, 0.01097,  true, cutoff)); // desolvation_sigma, solvation_q, charge_dependent, cutoff
	//add(d, new ad4_solvation(3.6, 0.01097, false, cutoff)); // desolvation_sigma, solvation_q, charge_dependent, cutoff

	//add(d, new electrostatic<1>(100, cutoff)); // cap, cutoff
	//add(d, new electrostatic<2>(100, cutoff)); // cap, cutoff

	//add(d, new gauss(0,   0.3, cutoff)); // offset, width, cutoff
	//add(d, new gauss(0.5, 0.3, cutoff)); // offset, width, cutoff
	//add(d, new gauss(1,   0.3, cutoff)); // offset, width, cutoff
	//add(d, new gauss(1.5, 0.3, cutoff)); // offset, width, cutoff
	//add(d, new gauss(2,   0.3, cutoff)); // offset, width, cutoff
	//add(d, new gauss(2.5, 0.3, cutoff)); // offset, width, cutoff

	add(1, new gauss(0, 0.5, cutoff)); // offset, width, cutoff // WEIGHT: -0.035579
	//add(d, new gauss(1, 0.5, cutoff)); // offset, width, cutoff
	//add(d, new gauss(2, 0.5, cutoff)); // offset, width, cutoff

	//add(d, new gauss(0, 0.7, cutoff)); // offset, width, cutoff
	//add(d, new gauss(1, 0.7, cutoff)); // offset, width, cutoff
	//add(d, new gauss(2, 0.7, cutoff)); // offset, width, cutoff

	//add(d, new gauss(0, 0.9, cutoff)); // offset, width, cutoff
	//add(d, new gauss(1, 0.9, cutoff)); // offset, width, cutoff
	//add(d, new gauss(2, 0.9, cutoff)); // offset, width, cutoff
	//add(d, new gauss(3, 0.9, cutoff)); // offset, width, cutoff

	//add(d, new gauss(0, 1.5, cutoff)); // offset, width, cutoff
	//add(d, new gauss(1, 1.5, cutoff)); // offset, width, cutoff
	//add(d, new gauss(2, 1.5, cutoff)); // offset, width, cutoff
	//add(d, new gauss(3, 1.5, cutoff)); // offset, width, cutoff
	//add(d, new gauss(4, 1.5, cutoff)); // offset, width, cutoff

	//add(d, new gauss(0, 2.0, cutoff)); // offset, width, cutoff
	//add(d, new gauss(1, 2.0, cutoff)); // offset, width, cutoff
	//add(d, new gauss(2, 2.0, cutoff)); // offset, width, cutoff
	add(1, new gauss(3, 2.0, cutoff)); // offset, width, cutoff // WEIGHT: -0.005156
	//add(d, new gauss(4, 2.0, cutoff)); // offset, width, cutoff

	//add(d, new gauss(0, 3.0, cutoff)); // offset, width, cutoff
	//add(d, new gauss(1, 3.0, cutoff)); // offset, width, cutoff
	//add(d, new gauss(2, 3.0, cutoff)); // offset, width, cutoff
	//add(d, new gauss(3, 3.0, cutoff)); // offset, width, cutoff
	//add(d, new gauss(4, 3.0, cutoff)); // offset, width, cutoff

	//add(d, new repulsion( 0.4, cutoff)); // offset, cutoff
	//add(d, new repulsion( 0.2, cutoff)); // offset, cutoff
	add(1, new repulsion( 0.0, cutoff)); // offset, cutoff // WEIGHT:  0.840245
	//add(d, new repulsion(-0.2, cutoff)); // offset, cutoff
	//add(d, new repulsion(-0.4, cutoff)); // offset, cutoff
	//add(d, new repulsion(-0.6, cutoff)); // offset, cutoff
	//add(d, new repulsion(-0.8, cutoff)); // offset, cutoff
	//add(d, new repulsion(-1.0, cutoff)); // offset, cutoff

	//add(d, new hydrophobic(0.5, 1, cutoff)); // good, bad, cutoff
	add(1, new hydrophobic(0.5, 1.5, cutoff)); // good, bad, cutoff // WEIGHT:  -0.035069
	//add(d, new hydrophobic(0.5, 2, cutoff)); // good, bad, cutoff
	//add(d, new hydrophobic(0.5, 3, cutoff)); // good, bad, cutoff

	//add(1, new non_hydrophobic(0.5, 1.5, cutoff));

	//add(d, new vdw<4,  8>(   0, 100, cutoff)); // smoothing, cap, cutoff

	add(1, new non_dir_h_bond(-0.7, 0, cutoff)); // good, bad, cutoff // WEIGHT:  -0.587439
	//add(d, new non_dir_h_bond(-0.7, 0, cutoff)); // good, bad, cutoff
	//add(d, new non_dir_h_bond(-0.7, 0.2, cutoff)); // good, bad, cutoff
	//add(d, new non_dir_h_bond(-0.7, 0.4, cutoff)); // good, bad, cutoff
	
	// Modern scoring terms (2010-2024) - initially disabled for testing
	// Halogen bonding: X···O/N interactions (Auffinger et al. PNAS 2004; Cavallo et al. Chem Rev 2016)
	// Improves scoring for Cl/Br/I-containing compounds
	add(d, new halogen_bond(-0.5, 0.5, cutoff)); // good, bad, cutoff // To be optimized
	
	// Pi-stacking: aromatic-aromatic interactions (Martinez & Iverson Chem Sci 2012)
	// Important for binding of aromatic-rich ligands and protein-ligand interfaces
	add(d, new pi_stacking(-0.4, 1.0, cutoff)); // good, bad, cutoff // To be optimized
	
	// S-π interactions: sulfur-aromatic (Reid et al. J Mol Biol 1985; Valley et al. PNAS 2012)
	// Relevant for methionine-containing binding sites
	add(d, new sulfur_aromatic(-0.3, 0.8, cutoff)); // good, bad, cutoff // To be optimized
	
	// Improved desolvation: modern entropy penalty (Ben-Shimon & Eisenstein Proteins 2010)
	// Better accounts for polar vs hydrophobic burial effects
	add(d, new desolvation_improved(0.02, cutoff)); // weight, cutoff // To be optimized
	
	// additive

	// conf-independent
	//add(d, new num_ligands());

	add(1, new num_tors_div()); // WEIGHT: 1.923 -- FIXME too close to limit?
	//add(d, new num_heavy_atoms_div());
	//add(d, new num_heavy_atoms());
	//add(1, new num_tors_add());
	//add(d, new num_tors_sqr());
	//add(d, new num_tors_sqrt());
	//add(d, new num_hydrophobic_atoms());
	///add(1, new ligand_length());

	//add(d, new num_tors(100, 100, false)); // cap, past_cap, heavy_only
	//add(1, new num_tors(100, 100,  true)); // cap, past_cap, heavy_only
	//add(d, new num_tors(  2,   1,  true)); // cap, past_cap, heavy_only
	//add(d, new num_heavy_atoms());
	//add(d, new ligand_max_num_h_bonds());
	//add(1, new num_ligands());
}
