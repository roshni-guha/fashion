import os
import requests
import pyvips

def download_and_slice(image_url, output_dir, tile_size=256):
    os.makedirs(output_dir, exist_ok=True)
    raw_image_path = os.path.join(output_dir, "raw_image.jpg")
    
    print(f"Downloading high-res image...")
    response = requests.get(image_url, timeout=60)
    response.raise_for_status()
    content_type = response.headers.get("Content-Type", "")
    if not content_type.startswith("image/"):
        raise ValueError(f"Expected an image from {image_url}, got {content_type!r}")
    with open(raw_image_path, 'wb') as f:
        f.write(response.content)
        
    print(f"Slicing into {tile_size}x{tile_size} Deep-Zoom pyramid...")
    
    # 'sequential' access streams the image through memory rather than loading it all at once
    img = pyvips.Image.new_from_file(raw_image_path, access='sequential')
    
    # dzsave automatically generates the folder structure and tiles
    dzi_path = os.path.join(output_dir, "fashion_item")
    img.dzsave(dzi_path, tile_size=tile_size, suffix=".png")
    
    print(f"Success! Tile pyramid saved to {dzi_path}_files/")

if __name__ == "__main__":
    # Example: House of Worth ball gown (1898) from the Met Costume Institute, public domain (object 156027)
    met_image_url = "https://images.metmuseum.org/CRDImages/ci/original/DP-1028-001.jpg"

    # Run the pipeline; save next to this script regardless of the working directory
    download_and_slice(met_image_url, os.path.join(os.path.dirname(os.path.abspath(__file__)), "dataset"))