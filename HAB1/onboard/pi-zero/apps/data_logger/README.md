# Integrated data logger

The final Pi application will:

- receive and preserve raw STM32 frames;
- decode versioned records for indexing and analysis;
- capture images or video from the Pi Camera;
- associate camera timestamps with STM32 mission time and sequence numbers;
- expose storage and camera failures; and
- use bounded files or rotation so storage exhaustion is manageable.

Implementation language and deployment method remain `TBD`.
