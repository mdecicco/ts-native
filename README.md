# ⚠️ Note to anyone who finds this ⚠️
In the interest of cleanliness, testability, maintainability this codebase is being rewritten as multiple smaller and more manageable ones which
can be found here: https://github.com/orgs/tsn-lang/repositories. This effort has the added bonus of affording me the space to give more
consideration to areas which I found to be pain points or thought could be improved. I'm putting more focus on testing but I'm only one man with
ADHD, and unit tests don't capture my interest... I think it'll be good enough but don't expect 100% coverage.

# TS Native
This is my attempt at creating a scripting language that I actually _want_ to use when working on video games and other hobby software projects.
No existing scripting languages that I could find really do it for me. Since reinventing wheels is one of my favorite pastimes I decided that
creating my own and modeling it after my favorite language was a worthwhile endeavor.

It's been five years since I started, and this is now the 4th iteration of this project. I've learned a lot along the way, and the last iteration
felt _so_ close to being what I wanted. It worked beautifully but I just didn't feel safe embarking on any new and exciting projects using it as
a foundational component. This 4th iteration still needs a lot of polish... and it shows... But I feel as though it is actually feasible to bring
it to the standard of quality that I dream of. I've put more work in by now than I think it would take to achieve this goal.

There's still some work to do before it's even in a usable state, but the experiences of the past make the path clear. Once I achieve first light
the main goal is to implement unit tests for everything I have so far, as well as documentation and general code clean up. This will be the key.

To get an idea about where this project is going, [check out where it's been.](https://github.com/mdecicco/ts-native/blob/before-overhaul/README.md)
If you do, keep in mind that the syntax has changed somewhat significantly. Now it is very near to being a subset of TypeScript, with the exception
of a few things that I feel TypeScript needs, and a few things that are necessities for working with raw memory in trusted scripts.

### Roadmap
- 🕑 Break up this repository into multiple smaller ones so that it's more manageable and testable
  - ☑️ Utilities
  - ☑️ API binding tools
  - ☑️ IR code generation (now includes code validation)
  - ☑️ Tokenizer (now more flexible, possibly slower but some optimization paths are clear)
  - ☑️ Parser
  - 🕑 Compiler
  - Backends
    - VM
    - JIT
  - Runtime
- Documentation
- VS Code extension
