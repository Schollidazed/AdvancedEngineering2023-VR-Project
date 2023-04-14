using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace PSXShaderKit
{
    public class PSXPostProcessEffect : MonoBehaviour
    {
        private enum ColorEmulationMode
        {
            Off = 0,
            Fullscreen_Customizable = 1,
            Fullscreen_Accurate = 2,
            PerObject_Accurate = 3
        };

        private enum DitheringMatrixSize
        {
            Dither2x2 = 0,
            Dither4x4 = 1,
            Dither4x4_PS1Pattern = 2
        }

        [Header("Color")]
        [SerializeField]
        [Tooltip("Whether to dither per-material on your objects, or full-screen as a post-process effect. Per-object uses a console-accurate dithering pattern.")]
        private ColorEmulationMode _ColorEmulationMode = ColorEmulationMode.PerObject_Accurate;
        [SerializeField]
        [Tooltip("The color depth, or amount of values per color channel. 256 is the console-accurate value to simulate a 24-bit color depth")]
        private Vector3 _FullscreenColorDepth = new Vector3(256, 256, 256);
        [SerializeField]
        [Tooltip("The color depth of the dithered output. Dithering happens after the color depth above is applied. 32 is the console-accurate value for a 15 bit color depth but doesn't look too good in linear color space.")]
        private Vector3 _FullscreenDitherDepth = new Vector3(32, 32, 32);
        [SerializeField]
        [Tooltip("The matrix size to use when dithering. 4x4 is higher quality with more patterns. The 4x4 matrix with the PS1 pattern is the most accurate but darkens the image compared to the rest.")]
        private DitheringMatrixSize _DitheringMatrixSize = DitheringMatrixSize.Dither4x4_PS1Pattern;

        [Header("Interlacing")]
        [SerializeField]
        [Tooltip("The amount of rows of pixels that get affected by interlacing. 1 is console-accurate but only works on lower resolutions.")]
        private int _InterlacingSize = 1;

        [Header("Shaders")]
        [SerializeField]
        private Shader _PostProcessShader;
        private Material _PostProcessMaterial;

        [SerializeField]
        private Shader _PostProcessShaderAccurate;
        private Material _PostProcessMaterialAccurate;

        private RenderTexture _CurrentFrame;

        [SerializeField]
        private Shader _InterlacingShader;
        private Material _InterlacingMaterial;
        private RenderTexture _PreviousFrame;

        private bool _IsFirstFrame = true;

        void Start()
        {
            if (_PostProcessShader != null && _PostProcessShader.isSupported)
            {
                _PostProcessMaterial = new Material(_PostProcessShader);
            }

            if (_PostProcessShaderAccurate != null && _PostProcessShaderAccurate.isSupported)
            {
                _PostProcessMaterialAccurate = new Material(_PostProcessShaderAccurate);
            }

            if (_InterlacingShader != null && _InterlacingShader.isSupported)
            {
                _InterlacingMaterial = new Material(_InterlacingShader);
            }
            else
            {
                _InterlacingSize = -1;
            }

            UpdateValues();
        }

        void OnValidate()
        {
            UpdateValues();
        }

        void UpdateValues()
        {
            Shader.SetGlobalFloat("_PSX_ObjectDithering", _ColorEmulationMode == ColorEmulationMode.PerObject_Accurate ? 1 : 0);
        }

        void OnDisable()
        {
            if (_CurrentFrame)
            {
                RenderTexture.ReleaseTemporary(_CurrentFrame);
            }

            if (_PreviousFrame)
            {
                RenderTexture.ReleaseTemporary(_PreviousFrame);
            }

            _IsFirstFrame = true;
        }

        void OnRenderImage(RenderTexture source, RenderTexture destination)
        {
            _PostProcessMaterial.SetVector("_ColorResolution", _FullscreenColorDepth);
            _PostProcessMaterial.SetVector("_DitherResolution", _FullscreenDitherDepth);
            switch (_DitheringMatrixSize)
            {
                case DitheringMatrixSize.Dither2x2:
                    _PostProcessMaterial.SetFloat("_HighResDitherMatrix", 0);
                    break;
                case DitheringMatrixSize.Dither4x4:
                    _PostProcessMaterial.SetFloat("_HighResDitherMatrix", 0.5f);
                    break;
                case DitheringMatrixSize.Dither4x4_PS1Pattern:
                    _PostProcessMaterial.SetFloat("_HighResDitherMatrix", 1.0f);
                    break;
            }

            RenderTexture postProcessDest;
            if(_InterlacingSize > 0)
            {
                if (_CurrentFrame)
                {
                    RenderTexture.ReleaseTemporary(_CurrentFrame);
                }
                _CurrentFrame = RenderTexture.GetTemporary(source.descriptor);
                postProcessDest = _CurrentFrame;
            }
            else
            {
                postProcessDest = destination;
            }

            switch (_ColorEmulationMode)
            {
                case ColorEmulationMode.Off:
                case ColorEmulationMode.PerObject_Accurate:
                    Graphics.Blit(source, postProcessDest);
                    break;
                case ColorEmulationMode.Fullscreen_Customizable:
                    Graphics.Blit(source, postProcessDest, _PostProcessMaterial);
                    break;
                case ColorEmulationMode.Fullscreen_Accurate:
                    Graphics.Blit(source, postProcessDest, _PostProcessMaterialAccurate);
                    break;
            }

            if (_InterlacingSize <= 0)
            {
                _IsFirstFrame = true;
            }
            else
            {
                _InterlacingMaterial.SetFloat("_InterlacedFrameIndex", Time.frameCount % 2);
                _InterlacingMaterial.SetFloat("_InterlacingSize", _InterlacingSize);
                _InterlacingMaterial.SetTexture("_PreviousFrame", _IsFirstFrame ? _CurrentFrame : _PreviousFrame);
                _IsFirstFrame = false;
                Graphics.Blit(_CurrentFrame, destination, _InterlacingMaterial);

                if (_PreviousFrame)
                {
                    RenderTexture.ReleaseTemporary(_PreviousFrame);
                }
                _PreviousFrame = RenderTexture.GetTemporary(source.descriptor);
                Graphics.Blit(_CurrentFrame, _PreviousFrame);

                RenderTexture.active = destination;
            }
        }
    }
}
