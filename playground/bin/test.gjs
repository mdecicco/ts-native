import { vec2f, random, atan2, min, max } from 'math';

class SearchGrid {
	constructor(i32 wDivs, i32 hDivs) : spaces() {
		this.wDivs = wDivs;
		this.hDivs = hDivs;
		u32 c = wDivs * hDivs;
		for (u32 s = 0;s < c;s++) {
			this.spaces.push(u32[]());
		}
	}

	void build(vec2f wsz, array<boid> boids) {
		u32 c = this.wDivs * this.hDivs;
		for (u32 s = 0;s < c;s++) this.spaces[s].clear();

		u32 l = boids.length;
		for (u32 b = 0;b < l;b++) {
			vec2f pos = boids[b].position;
			f32 xFrac = min(max(pos.x, 0.00001f), wsz.x - 1) / wsz.x;
			f32 yFrac = min(max(pos.y, 0.00001f), wsz.y - 1) / wsz.y;
			u32 gx = xFrac * this.wDivs;
			u32 gy = yFrac * this.hDivs;
			u32 idx = gx + (gy * this.wDivs);
			this.spaces[idx].push(b);
		}
	}

	void find_neighbors_in_cell(u32 gx, u32 gy, vec2f pos, f32 radius, array<boid> boids, array<u32> out, u32 selfIdx) {
		if (gx < 0 || gx >= this.wDivs) return;
		if (gy < 0 || gy >= this.hDivs) return;
		u32 idx = gx + (gy * this.wDivs);

		u32 l = this.spaces[idx].length;
		for (u32 i = 0;i < l;i++) {
			u32 bidx = this.spaces[idx][i];
			if (bidx != selfIdx) {
				if ((boids[bidx].position - pos).length <= radius) {
					out.push(bidx);
				}
			}
		}
	}

	void find_neighbors(vec2f wsz, vec2f pos, f32 radius, array<boid> boids, array<u32> out, u32 selfIdx) {
		f32 xFrac = min(max(pos.x, 0.00001f), wsz.x - 1) / wsz.x;
		f32 yFrac = min(max(pos.y, 0.00001f), wsz.y - 1) / wsz.y;
		u32 gx = xFrac * this.wDivs;
		u32 gy = yFrac * this.hDivs;
		this.find_neighbors_in_cell(gx - 1, gy - 1, pos, radius, boids, out, selfIdx);
		this.find_neighbors_in_cell(gx - 1, gy    , pos, radius, boids, out, selfIdx);
		this.find_neighbors_in_cell(gx - 1, gy + 1, pos, radius, boids, out, selfIdx);

		this.find_neighbors_in_cell(gx    , gy - 1, pos, radius, boids, out, selfIdx);
		this.find_neighbors_in_cell(gx    , gy    , pos, radius, boids, out, selfIdx);
		this.find_neighbors_in_cell(gx    , gy + 1, pos, radius, boids, out, selfIdx);

		this.find_neighbors_in_cell(gx + 1, gy - 1, pos, radius, boids, out, selfIdx);
		this.find_neighbors_in_cell(gx + 1, gy    , pos, radius, boids, out, selfIdx);
		this.find_neighbors_in_cell(gx + 1, gy + 1, pos, radius, boids, out, selfIdx);
	}
	
	u32[][] spaces;
	i32 wDivs;
	i32 hDivs;
};

boid create_boid(vec2f wsz) {
	boid b;
	b.position = random(vec2f(0.0f, 0.0f), wsz);
	b.velocity = random(vec2f(-70.0f, -70.0f), vec2f(70.0f, 70.0f));
	b.acceleration = vec2f(0.0f, 0.0f);
	b.wander = random(180.0f, -180.0f);
	return b;
}

void update_boid(f32 dt, boid b, u32 selfIdx, array<boid> boids, vec2f wsz, SearchGrid grid, array<u32> neighborIndices) {
	f32 maxSpeed = max_speed();
	f32 minSpeed = min_speed();
	f32 cohesionFac = cohesion_fac();
	f32 separationFac = separation_fac();
	f32 alignFac = align_fac();
	f32 wanderFac = wander_fac();
	f32 minDist = min_dist();
	f32 detectionRadius = detection_radius();

	neighborIndices.clear();
	grid.find_neighbors(wsz, b.position, detectionRadius, boids, neighborIndices, selfIdx);

	u32 neighborCount = neighborIndices.length;
	f32 nc = neighborCount;
	if (neighborCount > 0) {
		vec2f align = vec2f(0.0f, 0.0f);
		vec2f cohesion = vec2f(0.0f, 0.0f);
		vec2f separation = vec2f(0.0f, 0.0f);
		f32 sc = 0.0f;
		for (u32 n = 0;n < neighborCount;n++) {
			u32 bidx = neighborIndices[n];
			align += boids[bidx].velocity;
			cohesion += boids[bidx].position;
			vec2f delta = boids[bidx].position - b.position;
			f32 dist = delta.length;
			if (dist < minDist) {
				delta.normalize();
				delta /= dist;
				separation += delta;
				sc++;
			}
		}

		cohesion /= nc;
		cohesion -= b.position;
		cohesion.normalize();

		align /= nc;
		align.normalize();
		align *= maxSpeed;
		align -= b.velocity;
		
		if (sc > 0.0f && separation.lengthSq > 0.0f) {
			separation /= sc;
			separation.normalize();
			separation *= -maxSpeed;
			separation -= b.velocity;
		}

		b.acceleration += cohesion * cohesionFac;
		b.acceleration += align * alignFac;
		b.acceleration += separation * separationFac;
	}
	
	b.wander += random(-15.0f, 15.0f);
	vec2f wander = vec2f.fromAngleLength(b.wander, maxSpeed);
	wander *= max(0.0f, 5.0f - min(nc, 5.0f)) / 5.0f;
	b.acceleration += wander * wanderFac;

	b.velocity *= 0.8f;
	b.velocity += b.acceleration * dt;
	
	f32 speed = b.velocity.length;
	if (speed > maxSpeed) {
		b.velocity /= speed;
		b.velocity *= maxSpeed;
	}

	if (speed < minSpeed) {
		b.velocity /= speed;
		b.velocity *= minSpeed;
	}

	b.position += b.velocity * dt;
	if (b.position.x > wsz.x + 20.0f) b.position.x = -20.0f;
	if (b.position.y > wsz.y + 20.0f) b.position.y = -20.0f;
	if (b.position.x < -20.0f) b.position.x = wsz.x + 20.0f;
	if (b.position.y < -20.0f) b.position.y = wsz.y + 20.0f;
}

void main() {
	vec2f wsz = window_size();
	boid[] boids;
	for (u32 i = 0;i < 2000;i++) {
		boids.push(create_boid(wsz));
	}

	SearchGrid grid = SearchGrid(10, 10);
	u32[] neighborIndices;

	while (running()) {
		u32 count = agent_count();
		begin_frame();
		f32 dt = deltaT();
		wsz = window_size();
		grid.build(wsz, boids);

		u32 l = agent_count();
		for (u32 i = 0;i < l;i++) {
			update_boid(dt, boids[i], i, boids, wsz, grid, neighborIndices);
			draw_boid(boids[i]);
		}

		end_frame();
	}
}