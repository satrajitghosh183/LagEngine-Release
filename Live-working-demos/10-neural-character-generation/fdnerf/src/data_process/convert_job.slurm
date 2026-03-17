#!/bin/bash
#SBATCH --job-name=fdnerf        # create a short name for your job
#SBATCH --nodes=1                # node count
#SBATCH --ntasks=1               # total number of tasks across all nodes
#SBATCH --cpus-per-task=1       # cpu-cores per task (>1 if multi-threaded tasks)
#SBATCH --mem-per-cpu=4G         # memory per cpu-core (4G is default)         
#SBATCH --time=00:60:00          # total run time limit (HH:MM:SS) 
#SBATCH --mail-type=end          # send email when job ends
#SBATCH --mail-type=fail         # send email if job fails
#SBATCH --mail-user=<...>

module purge
module load anaconda3/2024.10
conda activate facenerf
cd ../..

# This scritp will convert the dataset from landmark detection to the format required by FDNeRF.
python -u ./src/data_process/fdnerf_converter.py \
    --input_dir /scratch/network/hy4522/FDNeRF_data/Custom/exp_result \
    --output_dir /scratch/network/hy4522/FDNeRF_data/Custom_converted \
    --generate_3dmm \

# nvidia-smi