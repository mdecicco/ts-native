# TS Native
This is my attempt at creating a scripting language that I actually _want_ to use when working on video games and other hobby software projects.
No existing scripting languages that I could find really do it for me. Since reinventing wheels is one of my favorite pastimes I decided that
creating my own and modeling it after my favorite language was a worthwhile endeavor.

It's been four years since I started, and this is now the 4th iteration of this project. I've learned a lot along the way, and the last iteration
felt _so_ close to being what I wanted. It worked beautifully but I just didn't feel safe embarking on any new and exciting projects using it as
a foundational component. This 4th iteration still needs a lot of polish... and it shows... But I feel as though it is actually feasible to bring
it to the standard of quality that I dream of. I've put more work in by now than I think it would take to achieve this goal.

There's still some work to do before it's even in a usable state, but the experiences of the past make the path clear. Once I achieve first light
the main goal is to implement unit tests for everything I have so far, as well as documentation and general code clean up. This will be the key.

To get an idea about where this project is going, [check out where it's been.](https://github.com/mdecicco/ts-native/blob/before-overhaul/README.md)
If you do, keep in mind that the syntax has changed somewhat significantly. Now it is very near to being a subset of TypeScript, with the exception
of a few things that I feel TypeScript needs, and a few things that are necessities for working with raw memory in trusted scripts.

### Roadmap
- ☑️ Reimplement the VM backend
- ☑️ Fix any bugs that arise in the implementation/testing phase of the VM
- ☑️ Clean up directory structure
- ☑️ Integrate memory management utilities better (currently require manual initialization)
- ❌ Implement project management tools for working in this repo (possibly using TSN)
- 🕑 Design some kind of structure for unit tests... I don't want tests and test data intermingled too much
- 🕑 Update existing unit tests, including for the 'utils' dependency
- Write/update documentation for all functions, classes, structures, enums, etc (Also including the 'utils' dependency)
  - (Implementing unit tests as I go)
- Write external documentation for TSN syntax and builtin language features
- Write external documentation for runtime behaviors and configuration
- Come up with a flexible development cycle and commit to it
- And finally... carefully consider the next most essential feature to implement
  - Implement that feature
  - Rinse and repeat

Once I have what I feel is a decent MVP, it will be time to shift my focus onto writing a proper VS Code plugin with *all* the fixings (including a
debugger). Only then will I be one of the _cool kids_. Don't even _look_ at the VSC plugins I have now, they are experimental toys I wrote to figure
out how feasible it is for me to write actual ones.
