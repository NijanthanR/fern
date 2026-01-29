use zed_extension_api as zed;

struct FernExtension {
    cached_binary_path: Option<String>,
}

impl zed::Extension for FernExtension {
    fn new() -> Self {
        FernExtension {
            cached_binary_path: None,
        }
    }

    fn language_server_command(
        &mut self,
        language_server_id: &zed::LanguageServerId,
        worktree: &zed::Worktree,
    ) -> zed::Result<zed::Command> {
        // Only handle fern-lsp
        if language_server_id.as_ref() != "fern-lsp" {
            return Err("Unknown language server".into());
        }

        // Try to find fern in PATH
        let fern_path = worktree
            .which("fern")
            .ok_or_else(|| "fern not found in PATH. Install fern or add it to your PATH.")?;

        self.cached_binary_path = Some(fern_path.clone());

        Ok(zed::Command {
            command: fern_path,
            args: vec!["lsp".to_string()],
            env: Default::default(),
        })
    }
}

zed::register_extension!(FernExtension);
