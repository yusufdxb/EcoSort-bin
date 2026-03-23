# Results Structure

This repo already reports classification accuracy on completed trials. The missing piece is to separate sensing accuracy from mechanical reliability in a way that is easy to audit.

## Publish Two Tables, Not One

### 1. Classification outcomes on completed trials

| Trial ID | Item type | Predicted class | Correct | Notes |
|---|---|---|---|---|
| C-01 | fill | fill | yes/no | |
| C-02 | fill | fill | yes/no | |

### 2. Mechanical reliability across all trials

| Trial ID | Jammed? | Stage of failure | Recovered automatically? | Notes |
|---|---|---|---|---|
| M-01 | yes/no | intake/sort/drop | yes/no | |
| M-02 | yes/no | intake/sort/drop | yes/no | |

## Recommended Summary Metrics

| Area | Metric |
|---|---|
| Classification | accuracy on completed trials |
| Reliability | jam rate across all trials |
| Reliability | successful sort completion rate |

## Interpretation Rule

Do not fold jammed trials into a vague headline number. Publicly separating the two is stronger and more credible.
