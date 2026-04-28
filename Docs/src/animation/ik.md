# Inverse Kinematics

LAG ships three skeletal IK solvers, all operating on an `AnimationPose` bone list.

| Solver | Use case | Complexity |
|--------|----------|-----------|
| `TwoBoneIK` | Arms, legs (exactly 3 bones) | Analytical, one-shot |
| `CCDIK` | General N-bone chains | Iterative, robust |
| `FABRIKSolver` | Long chains (tails, tentacles) | Iterative, fast |

## Registering chains

```cpp
IKSystem ik;
IKChainSpec leftArm;
leftArm.BoneIndices = { shoulderL, elbowL, wristL };
leftArm.TargetPosition = targetWorldPos;
leftArm.PoleVector = glm::vec3(0, 0, 1); // "elbow points this way"
ik.AddChain(leftArm, IKSystem::Method::TwoBone);

// In your animation update:
AnimationPose pose = blendTree.Evaluate(deltaTime);
ik.Apply(pose);        // writes IK-corrected bone transforms
```

## Two-bone IK

Solves analytically using the law of cosines:

1. Compute upper-segment + lower-segment lengths
2. Compute end-to-target distance
3. If unreachable → fully extend chain toward target
4. Otherwise → compute elbow angle from cosine rule
5. Apply pole vector to resolve the bend plane

## CCD (Cyclic Coordinate Descent)

Iteratively rotates each bone from end-effector-minus-one back to root,
each time minimizing end-effector-to-target distance. Converges for most
reasonable targets within 5–10 iterations.

## FABRIK

Two passes per iteration:

- **Backward** (end → root): snap end to target, pull each bone onto the
  line between itself and its child, preserving bone length
- **Forward** (root → end): snap root to original position, pull each
  bone onto the line toward its parent

Faster than CCD for long chains (10+ bones), stable behavior.
