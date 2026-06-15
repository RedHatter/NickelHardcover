use proc_macro::TokenStream;
use quote::quote;
use syn::{Data, DeriveInput, Fields, parse_macro_input};

#[proc_macro_derive(AggregateErrors)]
pub fn derive_aggregate_error(input: TokenStream) -> TokenStream {
  let input = parse_macro_input!(input as DeriveInput);
  let name = &input.ident;

  let tokens = if let Data::Struct(data_struct) = &input.data
    && let Fields::Named(fields) = &data_struct.fields
  {
    let body = if name == "ResponseData" {
      let mut field_errors = fields.named.iter().map(|field| {
        let field_name = &field.ident;
        quote! { self.#field_name.iter().flat_map(crate::utils::AggregateErrors::errors) }
      });

      let first_field = field_errors.next();

      quote! {
        #first_field
        #(.chain(#field_errors))*
      }
    } else if fields
      .named
      .iter()
      .find(|field| field.ident.as_ref().is_some_and(|name| name == "error"))
      .is_some()
    {
      quote! { self.error.iter().map(String::as_str) }
    } else if fields
      .named
      .iter()
      .find(|field| field.ident.as_ref().is_some_and(|name| name == "errors"))
      .is_some()
    {
      quote! { self.errors.iter().flatten().filter_map(Option::as_deref) }
    } else {
      quote! { std::iter::empty() }
    };

    quote! {
      impl crate::utils::AggregateErrors for #name {
        fn errors<'a>(&'a self) -> impl Iterator<Item = &'a str> {
          #body
        }
      }
    }
  } else {
    quote! {}
  };

  TokenStream::from(tokens)
}
