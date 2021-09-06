import { vec2f, cos, sin } from 'math';

void main() {
	boid b;
	b.pos = vec2f(200.0f, 200.0f);
	f32 x = 0.0f;
	while (dont_exit()) {
		x += 0.1f;
		b.facing.x = cos(x);
		b.facing.y = sin(x);
		begin_frame();
		for (u32 i = 0;i < 1;i++) {
			draw_boid(b);
		}
		end_frame();
	}
}