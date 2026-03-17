#!/bin/bash
#SBATCH --job-name=fdnerf        # create a short name for your job
#SBATCH --nodes=1                # node count
#SBATCH --ntasks=1               # total number of tasks across all nodes
#SBATCH --cpus-per-task=1       # cpu-cores per task (>1 if multi-threaded tasks)
#SBATCH --mem-per-cpu=4G         # memory per cpu-core (4G is default)
#SBATCH --gres=gpu:1          
#SBATCH --time=00:60:00          # total run time limit (HH:MM:SS) 
#SBATCH --mail-type=end          # send email when job ends
#SBATCH --mail-type=fail         # send email if job fails
#SBATCH --mail-user=<...>

export TORCH_HOME=/scratch/network/hy4522/.cache/torch # cache

module purge
module load anaconda3/2024.10
conda activate facenerf
cd ../..

# If the inputs are videos, you may first run this preprocessing script to convert them to images. FFmpeg is required.
python ./src/data_process/video_preprocess.py

python -u ./src/data_process/Wild_data_preprocess.py \
    --datapath /scratch/network/hy4522/FDNeRF_data/kiwi/processed \
    --savepath /scratch/network/hy4522/FDNeRF_data/kiwi/exp_result \
    --resolution_tar 512 \