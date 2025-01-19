function [energy, force] = find_force(N, neighbor_number, neighbor_list, box, r)
    [energy_repulsive,force_repulsive]=find_force_repulsive(N, neighbor_number, neighbor_list, box, r);
    [energy_band,force_band]=find_force_band(N, neighbor_number, neighbor_list, box, r);
    energy=energy_repulsive+energy_band;
    force=force_repulsive+force_band;
end
